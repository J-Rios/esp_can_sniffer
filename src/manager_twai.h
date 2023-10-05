/**
 * @file    manager_twai.h
 * @author  Jose Miguel Rios Rubio
 * @date    2023.10.03
 * @version 1.0.0
 * @brief   Manager that covers device main TWAI communication behaviour.
 */

/*****************************************************************************/

/* Include Guards Open */

#ifndef MANAGER_TWAI_H
#define MANAGER_TWAI_H

/*****************************************************************************/

/* Libraries */

// Standard C++ Libraries
#include <cstdint>

// FreeRTOS Libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*****************************************************************************/

/* Class Interface */

class Manager_Twai
{
    /******************************************************************/

    /* Public Constants */

    public:

    /******************************************************************/

    /* Private Constants */

    private:

        /**
         * @brief Component Tag for ESP-IDF Log messages.
         */
        static constexpr char TAG[] = "MANAGER_TWAI";

        /**
         * @brief Time between messages reception checks.
         */
        static const uint32_t T_CHECK_MSG_RX = 10U;

    /******************************************************************/

    /* Public Data Types */

    public:

    /******************************************************************/

    /* Private Data Types */

    private:

    /******************************************************************/

    /* Public Methods */

    public:

        /**
         * @brief Get the Singleton object instance (Instantiate if needed).
         * Note: This must be a static function.
         * @return Singleton* Current Singleton Object instance.
         */
        static Manager_Twai* get_instance();

        /**
         * @brief Manager Run.
         * @return true Run result success.
         * @return false Run result fail.
         */
        bool run();

    /******************************************************************/

    /* Private Methods */

    private:

        /**
         * @brief Construct a new Manager_Twai object.
         */
        Manager_Twai();

        static void task_twai_receive(void* arg);

        static void task_twai_alerts(void* arg);

    /******************************************************************/

    /* Public Attributes */

    public:

    /******************************************************************/

    /* Private Attributes */

    private:

        /**
         * @brief Static pointer to Singleton Object instance.
         */
        static Manager_Twai* instance;

    /******************************************************************/

};

/*****************************************************************************/

/* Include Guards Close */

#endif /* MANAGER_TWAI_H */
