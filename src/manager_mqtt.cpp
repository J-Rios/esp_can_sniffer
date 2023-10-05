/**
 * @file    manager_mqtt.cpp
 * @author  Jose Miguel Rios Rubio
 * @date    2023.10.03
 * @version 1.0.0
 * @brief   Manager that covers device MQTT communication behaviour.
 */

/*****************************************************************************/

/* Libraries */

// Header Interface
#include "manager_mqtt.h"

// Configuration File
#include "configuration.h"

// SDK Specific Libraries
#include "sdkconfig.h"

// Standard C++ Libraries
#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <new>

// FreeRTOS Libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/semphr.h"
//#include "freertos/queue.h"

// LWIP Libraries
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

// ESP-IDF Specififc Libraries
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_log.h"

// ESP Protocols Libraries
#include "mqtt_client.h"

// Application Components - Managers
#include "manager_wifi.h"

/*****************************************************************************/

/* MQTT Configuration */

static const char MQTT_BROKER_ADDRESS[] = CFG_MQTT_BROKER_ADDRESS;

/*****************************************************************************/

/* Class BSS Memory Reserved */

// Reserved static memory space in BSS section to instantiate the object
// through a placement new operator in Singleton get_instance() function
static uint8_t bss_memory_singleton[sizeof(Manager_MQTT)];

// Initialization of Manager_MQTT instance pointer (needed by linker)
Manager_MQTT* Manager_MQTT::instance = nullptr;

/*****************************************************************************/

/* In-Scope Elements */

/**
 * @brief MQTT CLient.
 */
static esp_mqtt_client_handle_t mqtt_client;

/*****************************************************************************/

/* In-Scope Function Prototypes */

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
    int32_t event_id, void *event_data);

static uint64_t systick();

/*****************************************************************************/

/* Class Constructor */

/**
 * @details
 * The Constructor of the class initializes all the internal attributes to
 * initial default values.
 */
Manager_MQTT::Manager_MQTT()
{
    instance = nullptr;
}

/*****************************************************************************/

/* Singleton Get Instance Method */

/**
 * @details
 * This function instantiate the object of this Singleton component if it
 * wasn't been instantiate, and return the address of the instantiated object.
 * The instantiation is done by a "placement new" operation that assign the
 * reserved static memory for the object from the BSS section (this avoid the
 * use of dynamic memory and the HEAP section usage).
 */
Manager_MQTT* Manager_MQTT::get_instance()
{
    if (instance == nullptr)
    {   instance = new(bss_memory_singleton) Manager_MQTT();   }

    return instance;
}

/*****************************************************************************/

/* Public Methods */

bool Manager_MQTT::run()
{
    // Manager Setup
    setup();

    // Create LEDs Behaviour Task
    xTaskCreatePinnedToCore(
        task_manager_mqtt,
        "manager_mqtt",
        TASK_STACK_MQTT,
        NULL,
        TASK_PRIORITY_MQTT,
        NULL,
        tskNO_AFFINITY
    );

    return true;
}

bool Manager_MQTT::publish(const char* topic, const char* msg, const int qos)
{
    int msg_id = -1;

    msg_id = esp_mqtt_client_publish(mqtt_client, topic, msg, 0, qos, 0);
    if (msg_id < 0)
    {
        ESP_LOGE(TAG, "Msg publish fail");
        return false;
    }

    return true;
}

void Manager_MQTT::mqtt_start()
{
    static esp_mqtt_client_config_t mqtt_cfg;
    mqtt_cfg.broker.address.uri = MQTT_BROKER_ADDRESS;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client,
        (esp_mqtt_event_id_t)(ESP_EVENT_ANY_ID), mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void Manager_MQTT::mqtt_stop()
{
    esp_mqtt_client_stop(mqtt_client);
    esp_mqtt_client_destroy(mqtt_client);
}

void Manager_MQTT::log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {   ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);   }
}

/*****************************************************************************/

/* Task - MQTT */

void Manager_MQTT::task_manager_mqtt(void* arg)
{
    //static const uint64_t T_PUBLISH_MSG_MS = 5000U;
    Manager_WiFi* ptr_manager_wifi = Manager_WiFi::get_instance();
    Manager_MQTT* ptr_manager_mqtt = Manager_MQTT::get_instance();

    //int64_t t0 = systick();
    bool wifi_was_connected = false;

    // Keep Task Active
    while (1)
    {
        // Release CPU usage from this task
        vTaskDelay(pdMS_TO_TICKS(10));

        // Do nothing if WiFi is not connected
        if (ptr_manager_wifi->wifi_connected == false)
        {
            if (wifi_was_connected)
            {   ptr_manager_mqtt->mqtt_stop();   }
            wifi_was_connected = false;
            continue;
        }

        // Publish WiFi connection message
        if (wifi_was_connected == false)
        {
            ptr_manager_mqtt->mqtt_start();
            wifi_was_connected = true;
        }

        // Periodically send a message
        //if (systick() - t0 > T_PUBLISH_MSG_MS)
        //{
        //    ptr_manager_mqtt->publish(
        //        "/esp_can_sniffer/out", "Device Alive", 1);
        //    t0 = systick();
        //}
    }
}

/*****************************************************************************/

/* MQTT Events Handler */

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
    int32_t event_id, void *event_data)
{
    Manager_MQTT* ptr_manager_mqtt = Manager_MQTT::get_instance();
    const char* TAG = ptr_manager_mqtt->TAG;

    ESP_LOGD(TAG, "Event base=%s, event_id=%" PRIi32 "", base, event_id);

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)(event_data);
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            const int QOS = 1;
            int msg_id = esp_mqtt_client_subscribe(client,
                "/esp_can_sniffer/cfg", QOS);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            ptr_manager_mqtt->publish(
                "/esp_can_sniffer/out", "Device Connected", 1);
            break;
        }

        case MQTT_EVENT_DISCONNECTED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        }

        case MQTT_EVENT_SUBSCRIBED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        }

        case MQTT_EVENT_UNSUBSCRIBED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        }

        case MQTT_EVENT_PUBLISHED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        }

        case MQTT_EVENT_DATA:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        }

        case MQTT_EVENT_ERROR:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            esp_mqtt_error_type_t error_type = event->error_handle->error_type;
            if (error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
            {
                ptr_manager_mqtt->log_error_if_nonzero(
                    "reported from esp-tls",
                    event->error_handle->esp_tls_last_esp_err);
                ptr_manager_mqtt->log_error_if_nonzero(
                    "reported from tls stack",
                    event->error_handle->esp_tls_stack_err);
                ptr_manager_mqtt->log_error_if_nonzero(
                    "captured as transport's socket errno",
                    event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)",
                    strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        }

        case MQTT_EVENT_BEFORE_CONNECT:
            break;

        case MQTT_USER_EVENT:
            break;

        case MQTT_EVENT_DELETED:
            ESP_LOGI(TAG, "MQTT Deleted");
            break;

        case MQTT_EVENT_ANY:
            break;

        default:
            ESP_LOGI(TAG, "Other event id: %d", event->event_id);
            break;
    }
}

/*****************************************************************************/

/* Private Methods */

bool Manager_MQTT::setup()
{
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);
    return true;
}

/*****************************************************************************/

/* In-Scope Functions */

static uint64_t systick()
{
    return ( (uint64_t)(esp_timer_get_time() / 1000) );
}

/*****************************************************************************/
