/**
 * @file    manager_leds.cpp
 * @author  Jose Miguel Rios Rubio
 * @date    2023.10.03
 * @version 1.0.0
 * @brief   Manager that covers device LEDs behaviour.
 */

/*****************************************************************************/

/* Libraries */

// Header Interface
#include "manager_leds.h"

// Configuration File
#include "configuration.h"

// SDK Specific Libraries
#include "sdkconfig.h"

// Standard C++ Libraries
#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <new>

// POSIX Libraries
//

// FreeRTOS Libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF Specififc Libraries
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_log.h"

// ESP-IDF Drivers Libraries
#include "driver/gpio.h"

/*****************************************************************************/

/* Class BSS Memory Reserved */

// Reserved static memory space in BSS section to instantiate the object
// through a placement new operator in Singleton get_instance() function
static uint8_t bss_memory_singleton[sizeof(Manager_Leds)];

// Initialization of Manager_Leds instance pointer (needed by linker)
Manager_Leds* Manager_Leds::instance = nullptr;

/*****************************************************************************/

/* In-Scope Function Prototypes */

//static void task_manager_leds(void* arg);

static uint64_t systick();

/*****************************************************************************/

/* Class Constructor */

/**
 * @details
 * The Constructor of the class initializes all the internal attributes to
 * initial default values.
 */
Manager_Leds::Manager_Leds()
:
    led_state(1U)
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
Manager_Leds* Manager_Leds::get_instance()
{
    if (instance == nullptr)
    {   instance = new(bss_memory_singleton) Manager_Leds();   }

    return instance;
}

/*****************************************************************************/

/* Public Methods */

bool Manager_Leds::run()
{
    // Initialize LEDs GPIOs to default values
    setup();

    // Create LEDs Behaviour Task
    xTaskCreatePinnedToCore(
        task_manager_leds,
        "manager_leds",
        TASK_STACK_LEDS,
        NULL,
        TASK_PRIORITY_LEDS,
        NULL,
        tskNO_AFFINITY
    );

    return true;
}

/*****************************************************************************/

/* Task - LEDs */

void Manager_Leds::task_manager_leds(void* arg)
{
    static const int64_t T_TOGGLE_LED_MS = 1000;

    Manager_Leds* ptr_manager_leds = Manager_Leds::get_instance();
    int64_t t0 = systick();

    while (1)
    {
        // Toggle LED periodically
        //if (systick() - t0 > T_TOGGLE_LED_MS)
        //{
        //    ptr_manager_leds->led_toggle();
        //    t0 = systick();
        //}

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*****************************************************************************/

/* Private Methods */

bool Manager_Leds::setup()
{
    // Initialize GPIOs
    ESP_ERROR_CHECK(gpio_reset_pin(GPIO_LED_RGB_R));
    ESP_ERROR_CHECK(gpio_reset_pin(GPIO_LED_RGB_G));
    ESP_ERROR_CHECK(gpio_reset_pin(GPIO_LED_RGB_B));
    ESP_ERROR_CHECK(gpio_reset_pin(GPIO_LED_COLD_WHITE));
    ESP_ERROR_CHECK(gpio_reset_pin(GPIO_LED_WARM_WHITE));

    // Set Digital Output GPIOs
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_LED_RGB_R, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_LED_RGB_R, 0U));
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_LED_RGB_G, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_LED_RGB_G, 0U));
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_LED_RGB_B, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_LED_RGB_B, 0U));
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_LED_COLD_WHITE, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_LED_COLD_WHITE, led_state));
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_LED_WARM_WHITE, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_LED_WARM_WHITE, 0U));

    return true;
}

void Manager_Leds::led_toggle()
{
    if (led_state == 0U)
    {   led_state = 1U;   }
    else
    {   led_state = 0U;   }

    gpio_set_level(GPIO_LED_COLD_WHITE, led_state);
}

/*****************************************************************************/

/* In-Scope Functions */

static uint64_t systick()
{
    return ( (uint64_t)(esp_timer_get_time() / 1000) );
}
