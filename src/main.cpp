
/* Libraries */

// SDK Specific Libraries
#include "sdkconfig.h"

// Standard C/C++ Libraries
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

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

/*****************************************************************************/

/* Function Prototypes */

extern "C" { void app_main(void); }
static void twai_receive_task(void* arg);
static void twai_alerts_task(void* arg);

/*****************************************************************************/

/* Defines and Configurations */

// GPIOs
#define GPIO_TWIO_RX                    GPIO_NUM_6
#define GPIO_TWIO_TX                    GPIO_NUM_7

// Tasks priority
#define RX_TASK_PRIO                    9

// Time between messages reception checks
static const uint32_t T_CHECK_MSG_RX = 10U;

/*****************************************************************************/

/* In-Scope Global Elements */

// TWAI Timing-Speed Configuration
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

// TWAI Filter to accept all kind of messages IDs
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

// TWAI General Configuration
// Set TX queue length to 0 due to listen only mode
static const twai_general_config_t g_config =
{
    .mode = TWAI_MODE_LISTEN_ONLY,
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

/* Main Function */

void app_main()
{
    static const char TAG[] = "MAIN";

    ESP_LOGI(TAG, "App Start");

    // Initialize TWAI driver
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(TAG, "TWAI driver initialized");

    // Start TWAI
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "TWAI started");

    // Create TWAI Messages Reception Handler Task
    xTaskCreatePinnedToCore(
        twai_alerts_task,
        "TWAI_alerts",
        4096U,
        NULL,
        RX_TASK_PRIO,
        NULL,
        tskNO_AFFINITY
    );

    // Create TWAI Messages Reception Handler Task
    xTaskCreatePinnedToCore(
        twai_receive_task,
        "TWAI_rx",
        4096U,
        NULL,
        RX_TASK_PRIO,
        NULL,
        tskNO_AFFINITY
    );

    // Keep Main Task alive
    //while (1)
    //{   vTaskDelay(pdMS_TO_TICKS(100));   }
}

/*****************************************************************************/

/* TWAI Messages Reception Handler Task */

static void twai_receive_task(void* arg)
{
    struct timeval tv_now;
    twai_message_t msg;

    while (1)
    {
        // Check for message reception
        if (twai_receive(&msg, pdMS_TO_TICKS(T_CHECK_MSG_RX)) != ESP_OK)
        {   continue;   }

        // Get current timestamp (us resolution)
        gettimeofday(&tv_now, NULL);
        int64_t time_us = (int64_t)(tv_now.tv_sec * 1000000L) +
            (int64_t)tv_now.tv_usec;

        //Process received message
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
    }
}

static void twai_alerts_task(void* arg)
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
