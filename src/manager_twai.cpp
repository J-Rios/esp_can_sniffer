/**
 * @file    Manager_Twai.cpp
 * @author  Jose Miguel Rios Rubio
 * @date    2023.10.03
 * @version 1.0.0
 * @brief   Manager that covers device main TWAI communication behaviour.
 */

/*****************************************************************************/

/* Libraries */

// Header Interface
#include "Manager_Twai.h"

// Configuration File
#include "configuration.h"

// SDK Specific Libraries
#include "sdkconfig.h"

// Standard C++ Libraries
#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <cstring>
#include <new>

// POSIX Libraries
#include <sys/time.h>

// FreeRTOS Libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF Specififc Libraries
#include "esp_err.h"
#include "esp_log.h"

// ESP-IDF Drivers Libraries
#include "driver/twai.h"

// Application Components - Managers
#include "manager_leds.h"
#include "manager_mqtt.h"

/*****************************************************************************/

/* Class BSS Memory Reserved */

// Reserved static memory space in BSS section to instantiate the object
// through a placement new operator in Singleton get_instance() function
static uint8_t bss_memory_singleton[sizeof(Manager_Twai)];

// Initialization of Manager_Twai instance pointer (needed by linker)
Manager_Twai* Manager_Twai::instance = nullptr;

/*****************************************************************************/

/* In-Scope Global Elements */

// TWAI Timing-Speed Configuration
static const twai_timing_config_t t_config = TWAI_SPEED();

// TWAI Filter to accept all kind of messages IDs
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

// TWAI General Configuration
// Set TX queue length to 0 due to listen only mode
static const twai_general_config_t g_config =
{
    .mode = TWAI_MODE,
    .tx_io = GPIO_TWIO_TX,
    .rx_io = GPIO_TWIO_RX,
    .clkout_io = TWAI_IO_UNUSED,
    .bus_off_io = TWAI_IO_UNUSED,
    .tx_queue_len = 0,
    .rx_queue_len = 20,
    .alerts_enabled = TWAI_ALERT_NONE,
    .clkout_divider = 0,
    .intr_flags = ESP_INTR_FLAG_IRAM,
};

/*****************************************************************************/

/* Class Constructor */

/**
 * @details
 * The Constructor of the class initializes all the internal attributes to
 * initial default values.
 */
Manager_Twai::Manager_Twai()
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
Manager_Twai* Manager_Twai::get_instance()
{
    if (instance == nullptr)
    {   instance = new(bss_memory_singleton) Manager_Twai();   }

    return instance;
}

/*****************************************************************************/

/* Public Methods */

bool Manager_Twai::run()
{
    // Initialize TWAI driver
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(TAG, "TWAI driver initialized");

    // Start TWAI
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "TWAI started");

    // Create TWAI Messages Reception Handler Task
    xTaskCreatePinnedToCore(
        task_twai_alerts,
        "TWAI_alerts",
        TASK_STACK_TWIO_ALERTS,
        NULL,
        TASK_PRIORITY_TWIO_ALERTS,
        NULL,
        tskNO_AFFINITY
    );

    // Create TWAI Messages Reception Handler Task
    xTaskCreatePinnedToCore(
        task_twai_receive,
        "TWAI_rx",
        TASK_STACK_TWIO_RX,
        NULL,
        TASK_PRIORITY_TWIO_RX,
        NULL,
        tskNO_AFFINITY
    );

    return true;
}

/*****************************************************************************/

/* private Methods */


/*****************************************************************************/

/* Task - TWAI Messages Reception Handler */

void Manager_Twai::task_twai_receive(void* arg)
{
    struct timeval tv_now;
    twai_message_t msg;

    Manager_Leds* ManagerLEDs = Manager_Leds::get_instance();
    Manager_MQTT* ManagerMQTT = Manager_MQTT::get_instance();

    while (1)
    {
        // Check for message reception
        if (twai_receive(&msg, pdMS_TO_TICKS(T_CHECK_MSG_RX)) != ESP_OK)
        {   continue;   }

        // Get current timestamp (us resolution)
        gettimeofday(&tv_now, NULL);
        int64_t time_us = (int64_t)(tv_now.tv_sec * 1000000L) +
            (int64_t)tv_now.tv_usec;

        // Show received message through Serial
        printf("\n");
        printf("Message Received:\n");
        printf("  Time (us): %" PRId64 "\n", time_us);
        printf("  Frame: ");
        if (msg.extd)
        {
            printf("Extended\n");
            printf("  ID: 0x%08X\n", (unsigned int)(msg.identifier));
        }
        else
        {
            printf("Standard\n");
            printf("  ID: 0x%03X\n", (unsigned int)(msg.identifier));
        }
        printf("  Length: %u\n", (unsigned)msg.data_length_code);
        printf("  Data: ");
        if (msg.rtr)
        {   printf("NONE");   }
        else
        {
            for (int i = 0; i < msg.data_length_code; i++)
            {   printf("%02X", msg.data[i]);   }
        }
        printf("\n\n");

        // Send Received message through MQTT
        static const char MQTT_TWAI_TOPIC[] = "/esp_can_sniffer/twai/rx";
        char msg_rx_text[128];
        char frame_type[10];
        if (msg.extd)
        {   snprintf(frame_type, 10, "%s", "Extended");   }
        else
        {   snprintf(frame_type, 10, "%s", "Standard");   }
            snprintf(msg_rx_text, 128,
                (const char*)(
                    "Time (us): %" PRId64 "\n"
                    "Frame: %s\n"
                    "ID: 0x%08X\n"
                    "Length: %u\n"
                    "Data: "
                ),
                time_us,
                frame_type,
                (unsigned int)(msg.identifier),
                (unsigned)msg.data_length_code);
        if (msg.rtr)
        {   strncat(msg_rx_text, "NONE", 5U);   }
        else
        {
            char byte_str[3];
            for (int i = 0; i < msg.data_length_code; i++)
            {
                snprintf(byte_str, 3, "%02X", msg.data[i]);
                strncat(msg_rx_text, byte_str, 2);
            }
        }

        ManagerMQTT->publish(MQTT_TWAI_TOPIC, msg_rx_text, 0);
    }
}

void Manager_Twai::task_twai_alerts(void* arg)
{
    static const char TAG[] = "TWAI_ALERTS";

    // Configure TWIO alerts
    static const uint32_t enable_alerts =
    (
        // Bus error (Bit, Stuff, CRC, Form, ACK) has occurred
        TWAI_ALERT_BUS_ERROR |

        // TWAI controller has become error active
        TWAI_ALERT_ERR_ACTIVE |

        // TWAI controller has become error passive
        TWAI_ALERT_ERR_PASS |

        // Bus-off condition occurred
        TWAI_ALERT_BUS_OFF |

        // TWAI controller is undergoing bus recovery
        TWAI_ALERT_RECOVERY_IN_PROGRESS |

        // TWAI controller has successfully completed bus recovery
        TWAI_ALERT_BUS_RECOVERED |

        // A frame has been received and added to the RX queue
        //TWAI_ALERT_RX_DATA |

        // The RX queue is full causing a received frame to be lost
        TWAI_ALERT_RX_QUEUE_FULL |

        // No more messages queued for transmission
        TWAI_ALERT_TX_IDLE |

        // The previous transmission was successful
        TWAI_ALERT_TX_SUCCESS |

        // The previous transmission has failed
        TWAI_ALERT_TX_FAILED |

        // The previous transmission lost arbitration
        TWAI_ALERT_ARB_LOST |

        // An error counters have exceeded the error warning limit
        TWAI_ALERT_ABOVE_ERR_WARN |

        // Both error counters have dropped below error warning limit
        TWAI_ALERT_BELOW_ERR_WARN
    );

    // TWAI alerts configuration
    if (twai_reconfigure_alerts(enable_alerts, NULL) == ESP_OK)
    {   ESP_LOGI(TAG, "TWIO Alerts configured");   }
    else
    {   ESP_LOGE(TAG, "TWIO Fail to config alerts");   }

    while (1)
    {
        uint32_t alerts = 0U;

        // Check for message reception
        if (twai_read_alerts(&alerts, pdMS_TO_TICKS(T_CHECK_MSG_RX)) != ESP_OK)
        {   continue;   }

        // Bus error (Bit, Stuff, CRC, Form, ACK) has occurred
        if (alerts == TWAI_ALERT_BUS_ERROR)
        {   ESP_LOGE(TAG, "TWAI Bus Error");   }

        // TWAI controller has become error active
        else if (alerts == TWAI_ALERT_ERR_ACTIVE)
        {   ESP_LOGE(TAG, "TWAI Error Active");   }

        // TWAI controller has become error passive
        else if (alerts == TWAI_ALERT_ERR_PASS)
        {   ESP_LOGE(TAG, "TWAI Error Passive");   }

        // Bus-off condition occurred, launch Bus recovery
        else if (alerts == TWAI_ALERT_BUS_OFF)
        {
            ESP_LOGE(TAG, "TWAI Bus Off");
            ESP_ERROR_CHECK(twai_initiate_recovery());
        }

        // TWAI controller is undergoing bus recovery
        else if (alerts == TWAI_ALERT_RECOVERY_IN_PROGRESS)
        {   ESP_LOGI(TAG, "TWAI Bus Recovering...");   }

        // TWAI controller has successfully completed bus recovery
        else if (alerts == TWAI_ALERT_BUS_RECOVERED)
        {
            ESP_LOGI(TAG, "TWAI Bus Recovered");
            ESP_ERROR_CHECK(twai_start());
        }

        // A frame has been received and added to the RX queue
        else if (alerts == TWAI_ALERT_RX_DATA)
        {   ESP_LOGI(TAG, "TWAI Msg Rx");   }

        // The RX queue is full causing a received frame to be lost
        else if (alerts == TWAI_ALERT_RX_QUEUE_FULL)
        {   ESP_LOGW(TAG, "TWAI Rx queue full");   }

        // No more messages queued for transmission
        else if (alerts == TWAI_ALERT_TX_IDLE)
        {   ESP_LOGI(TAG, "TWAI Tx queue empty");   }

        // The previous transmission was successful
        else if (alerts == TWAI_ALERT_TX_SUCCESS)
        {   ESP_LOGI(TAG, "TWAI Msg Tx");   }

        // The previous transmission has failed
        else if (alerts == TWAI_ALERT_TX_FAILED)
        {   ESP_LOGE(TAG, "TWAI Tx fail");   }

        // The previous transmission lost arbitration
        else if (alerts == TWAI_ALERT_ARB_LOST)
        {   ESP_LOGE(TAG, "TWAI Tx lost arbitration");   }

        // An error counters have exceeded the error warning limit
        else if (alerts == TWAI_ALERT_ABOVE_ERR_WARN)
        {   ESP_LOGE(TAG, "TWAI error count overflow");   }

        // Both error counters have dropped below error warning limit
        else if (alerts == TWAI_ALERT_BELOW_ERR_WARN)
        {   ESP_LOGE(TAG, "TWAI error count drop");   }
    }
}

/*****************************************************************************/
