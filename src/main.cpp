
/* Libraries */

// SDK Specific Libraries
#include "sdkconfig.h"

// Standard C++ Libraries
#include <cstdio>
#include <cstdlib>
#include <cinttypes>

// FreeRTOS Libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF Specififc Libraries
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Application Components - Managers
#include "manager_leds.h"
#include "manager_mqtt.h"
#include "manager_twai.h"
#include "manager_wifi.h"

/*****************************************************************************/

/* Function Prototypes */

extern "C" { void app_main(void); }

/*****************************************************************************/

/* Main Function */

void app_main()
{
    static const char TAG[] = "MAIN";

    ESP_LOGI(TAG, "App Start");

    Manager_Twai* ManagerTWAI = Manager_Twai::get_instance();
    Manager_Leds* ManagerLEDs = Manager_Leds::get_instance();
    Manager_WiFi* ManagerWIFI = Manager_WiFi::get_instance();
    Manager_MQTT* ManagerMQTT = Manager_MQTT::get_instance();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if ( (ret == ESP_ERR_NVS_NO_FREE_PAGES)
      || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND) )
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Launch Managers
    ManagerTWAI->run();
    ManagerLEDs->run();
    ManagerWIFI->run();
    ManagerMQTT->run();
}
