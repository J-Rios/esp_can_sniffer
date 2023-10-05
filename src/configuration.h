
/* Include Guards Open */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

/*****************************************************************************/

/* Libraries */

// Standard C/C++ Libraries
#include <stdint.h>

/*****************************************************************************/

/* Project Configuration */

// GPIOs
#define GPIO_LED_RGB_R                  GPIO_NUM_3
#define GPIO_LED_RGB_G                  GPIO_NUM_4
#define GPIO_LED_RGB_B                  GPIO_NUM_5
#define GPIO_TWIO_TX                    GPIO_NUM_6
#define GPIO_TWIO_RX                    GPIO_NUM_7
#define GPIO_LED_WARM_WHITE             GPIO_NUM_18
#define GPIO_LED_COLD_WHITE             GPIO_NUM_19

// WiFi Station Connection
#if !defined(CFG_WIFI_SSID)
    #define CFG_WIFI_SSID "MySSID"
#endif
#if !defined(CFG_WIFI_PASSWORD)
    #define CFG_WIFI_PASSWORD "MyPassword1234"
#endif

#if !defined(CFG_MQTT_BROKER_ADDRESS)
    #define CFG_MQTT_BROKER_ADDRESS "mqtt://test.mosquitto.org"
#endif

// TWAI Configuration
#define TWAI_SPEED                      TWAI_TIMING_CONFIG_500KBITS
#define TWAI_MODE                       TWAI_MODE_LISTEN_ONLY

// Tasks priority
#define TASK_PRIORITY_TWIO_ALERTS       9
#define TASK_PRIORITY_TWIO_RX           9
#define TASK_PRIORITY_LEDS              9
#define TASK_PRIORITY_WIFI              9
#define TASK_PRIORITY_MQTT              9

// Tasks stack sizes
#define TASK_STACK_TWIO_ALERTS          4096U
#define TASK_STACK_TWIO_RX              4096U
#define TASK_STACK_LEDS                 4096U
#define TASK_STACK_WIFI                 8192U
#define TASK_STACK_MQTT                 8192U

/*****************************************************************************/

/* Include Guards Close */

#endif /* CONFIGURATION_H */
