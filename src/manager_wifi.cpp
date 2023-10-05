/**
 * @file    manager_wifi.cpp
 * @author  Jose Miguel Rios Rubio
 * @date    2023.10.03
 * @version 1.0.0
 * @brief   Manager that covers device WiFi behaviour.
 */

/*****************************************************************************/

/* Libraries */

// Header Interface
#include "manager_wifi.h"

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
#include "freertos/event_groups.h"

// LWIP Libraries
#include "lwip/err.h"
#include "lwip/sys.h"

// ESP-IDF Specififc Libraries
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"

// ESP-IDF Drivers Libraries
//

/*****************************************************************************/

/* WiFi Configuration */

static const char WIFI_STA_SSID[] = CFG_WIFI_SSID;
static const char WIFI_STA_PASSWORD[] = CFG_WIFI_PASSWORD;

#define WIFI_STA_MAX_RETRIES 10U

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
    #define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
    #define H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
    #define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
    #define H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
    #define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
    #define H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#else
    #define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
    #define H2E_IDENTIFIER ""
#endif

#if CONFIG_ESP_WIFI_AUTH_OPEN
    #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
    #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
    #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
    #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
    #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
    #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
    #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
    #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#else
    #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#endif

/*****************************************************************************/

/* Class BSS Memory Reserved */

// Reserved static memory space in BSS section to instantiate the object
// through a placement new operator in Singleton get_instance() function
static uint8_t bss_memory_singleton[sizeof(Manager_WiFi)];

// Initialization of Manager_WiFi instance pointer (needed by linker)
Manager_WiFi* Manager_WiFi::instance = nullptr;

/*****************************************************************************/

/* In-Scope Elements */

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/*****************************************************************************/

/* In-Scope Function Prototypes */

static void event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data);

/*****************************************************************************/

/* Class Constructor */

/**
 * @details
 * The Constructor of the class initializes all the internal attributes to
 * initial default values.
 */
Manager_WiFi::Manager_WiFi()
:
    wifi_connected(false)
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
Manager_WiFi* Manager_WiFi::get_instance()
{
    if (instance == nullptr)
    {   instance = new(bss_memory_singleton) Manager_WiFi();   }

    return instance;
}

/*****************************************************************************/

/* Public Methods */

bool Manager_WiFi::run()
{
    // Manager Setup
    setup();

    // Create LEDs Behaviour Task
    xTaskCreatePinnedToCore(
        task_manager_wifi,
        "manager_wifi",
        TASK_STACK_WIFI,
        NULL,
        TASK_PRIORITY_WIFI,
        NULL,
        tskNO_AFFINITY
    );

    return true;
}

/*****************************************************************************/

/* Task - WiFi */

void Manager_WiFi::task_manager_wifi(void* arg)
{
    Manager_WiFi* ptr_manager_wifi = Manager_WiFi::get_instance();

    ESP_LOGI(TAG, "WiFi Station init");
    ptr_manager_wifi->wifi_init_sta();

    // Keep Task alive
    while (1)
    {
        //if (ptr_manager_wifi->wifi_connected)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*****************************************************************************/

/* WiFi Events Handler */

static void event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    static int s_retry_num = 0;
    Manager_WiFi* ptr_manager_wifi = Manager_WiFi::get_instance();
    const char* TAG = ptr_manager_wifi->TAG;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {   esp_wifi_connect();   }

    else if ( (event_base == WIFI_EVENT)
        &&    (event_id == WIFI_EVENT_STA_DISCONNECTED) )
    {
        ESP_LOGI(TAG, "WiFi Disconnected");
        if (s_retry_num < WIFI_STA_MAX_RETRIES)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {   xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);   }
        ESP_LOGI(TAG,"connect to the AP fail");
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        ptr_manager_wifi->wifi_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }

    else
    {   ptr_manager_wifi->wifi_connected = false;   }
}

/*****************************************************************************/

/* Private Methods */

bool Manager_WiFi::setup()
{
    return true;
}

void Manager_WiFi::wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    static wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL,
            &instance_any_id
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &event_handler,
            NULL,
            &instance_got_ip
        )
    );

    #if 0
        wifi_config_t wifi_config =
    {
        .sta =
        {
            .ssid = WIFI_STA_SSID,
            .password = WIFI_STA_PASSWORD,
            /* Authmode threshold resets to WPA2 as default if password
             * matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA
             * networks, Please set the threshold value to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with
             * length and format matching to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK
             * standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = H2E_IDENTIFIER,
        },
    };
    #endif

    static wifi_config_t wifi_config;
    snprintf((char*)(wifi_config.sta.ssid), sizeof(wifi_config.sta.ssid),
        "%s", WIFI_STA_SSID);
    snprintf((char*)(wifi_config.sta.password),
        sizeof(wifi_config.sta.password), "%s", WIFI_STA_PASSWORD);
    wifi_config.sta.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD;
    wifi_config.sta.sae_pwe_h2e = ESP_WIFI_SAE_MODE;
    snprintf((char*)(wifi_config.sta.sae_h2e_identifier),
        sizeof(wifi_config.sta.sae_h2e_identifier), "%s", H2E_IDENTIFIER);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT)
     * or connection failed for the maximum number of re-tries
     * (WIFI_FAIL_BIT). The bits are set by event_handler() (see above)
     */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence
     * we can test which event actually happened.
     */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_STA_SSID, WIFI_STA_PASSWORD);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_STA_SSID, WIFI_STA_PASSWORD);
    }
    else
    {   ESP_LOGE(TAG, "UNEXPECTED EVENT");   }
}

/*****************************************************************************/
