/**
 * @file    manager_leds.h
 * @author  Jose Miguel Rios Rubio
 * @date    2023.10.03
 * @version 1.0.0
 * @brief   Manager that covers device LEDs behaviour.
 */

/*****************************************************************************/

/* Include Guards */

#ifndef MANAGER_LEDS_H
#define MANAGER_LEDS_H

/*****************************************************************************/

/* Libraries */

// Standard C++ Libraries
#include <cstdint>

/*****************************************************************************/

/* Class Interface */

class Manager_Leds
{
    /******************************************************************/

    /* Private Constants */

    private:

        /**
         * @brief Component Tag for ESP-IDF Log messages.
         */
        static constexpr char TAG[] = "MANAGER_LEDS";

    /******************************************************************/

    /* Public Constants */

    public:



    /******************************************************************/

    /* Private Data Types */

    private:



    /******************************************************************/

    /* Public Data Types */

    public:



    /******************************************************************/

    /* Public Methods */

    public:

        /**
         * @brief Get the Singleton object instance (Instantiate if needed).
         * Note: This must be a static function.
         * @return Singleton* Current Singleton Object instance.
         */
        static Manager_Leds* get_instance();

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
         * @brief Construct a new Manager_Leds object.
         * Note: This must be private to be Singleton.
         */
        Manager_Leds();

        /**
         * @brief Manager setup (set LED GPIOs initial status).
         * @return true Setup success.
         * @return false Setup fail.
         */
        bool setup();

        /**
         * @brief Toggle main LED status.
         */
        void led_toggle();

        static void task_manager_leds(void* arg);

    /******************************************************************/

    /* Public Attributes */

    public:



    /******************************************************************/

    /* Private Attributes */

    private:

        /**
         * @brief Static pointer to Singleton Object instance.
         */
        static Manager_Leds* instance;

        /**
         * @brief LED value.
         */
        uint8_t led_state;

    /******************************************************************/
};

/*****************************************************************************/

/* Include Guards Close */

#endif /* MANAGER_LEDS_H */
