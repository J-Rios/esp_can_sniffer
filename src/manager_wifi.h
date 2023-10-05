/**
 * @file    manager_wifi.h
 * @author  Jose Miguel Rios Rubio
 * @date    2023.10.03
 * @version 1.0.0
 * @brief   Manager that covers device WiFi behaviour.
 */

/*****************************************************************************/

/* Include Guards */

#ifndef MANAGER_WIFI_H
#define MANAGER_WIFI_H

/*****************************************************************************/

/* Libraries */

// Standard C++ Libraries
#include <cstdint>

/*****************************************************************************/

/* Class Interface */

class Manager_WiFi
{
    /******************************************************************/

    /* Private Constants */

    private:



    /******************************************************************/

    /* Public Constants */

    public:

        /**
         * @brief Component Tag for ESP-IDF Log messages.
         */
        static constexpr char TAG[] = "MANAGER_WiFi";

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
        static Manager_WiFi* get_instance();

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
         * @brief Construct a new Manager_WiFi object.
         * Note: This must be private to be Singleton.
         */
        Manager_WiFi();

        /**
         * @brief Manager setup.
         * @return true Setup success.
         * @return false Setup fail.
         */
        bool setup();

        void wifi_init_sta();

        /**
         * @brief Manager main task.
         * @param arg Task arguments that can be received.
         */
        static void task_manager_wifi(void* arg);

    /******************************************************************/

    /* Public Attributes */

    public:

        /**
         * @brief WiFi connection status.
         */
        bool wifi_connected;

    /******************************************************************/

    /* Private Attributes */

    private:

        /**
         * @brief Static pointer to Singleton Object instance.
         */
        static Manager_WiFi* instance;

    /******************************************************************/
};

/*****************************************************************************/

/* Include Guards Close */

#endif /* MANAGER_WIFI_H */
