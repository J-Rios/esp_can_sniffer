/**
 * @file    manager_mqtt.h
 * @author  Jose Miguel Rios Rubio
 * @date    2023.10.03
 * @version 1.0.0
 * @brief   Manager that covers device TTTT behaviour.
 */

/*****************************************************************************/

/* Include Guards */

#ifndef MANAGER_MQTT_H
#define MANAGER_MQTT_H

/*****************************************************************************/

/* Libraries */

// Standard C++ Libraries
#include <cstdint>

/*****************************************************************************/

/* Class Interface */

class Manager_MQTT
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
        static constexpr char TAG[] = "MANAGER_MQTT";

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
        static Manager_MQTT* get_instance();

        /**
         * @brief Manager Run.
         * @return true Run result success.
         * @return false Run result fail.
         */
        bool run();

        /**
         * @brief Publish a MQTT message to the specified topic and QoS level.
         * @param topic Topic to publish the message.
         * @param msg Text of the message data to publish.
         * @param qos Quality of Service level value.
         * @return true Publish success.
         * @return false Publish fail.
         */
        bool publish(const char* topic, const char* msg, const int qos);

        /**
         * @brief Initialize and launch MQTT Client.
         */
        void mqtt_start();

        /**
         * @brief Stop MQTT Client.
         */
        void mqtt_stop();

        /**
         * @brief Show non-zero errors through log interface.
         * @param message Message text to show.
         * @param error_code Error code number to show.
         */
        void log_error_if_nonzero(const char *message, int error_code);

    /******************************************************************/

    /* Private Methods */

    private:

        /**
         * @brief Construct a new Manager_MQTT object.
         * Note: This must be private to be Singleton.
         */
        Manager_MQTT();

        /**
         * @brief Manager setup.
         * @return true Setup success.
         * @return false Setup fail.
         */
        bool setup();

        /**
         * @brief Manager main task.
         * @param arg Task arguments that can be received.
         */
        static void task_manager_mqtt(void* arg);

    /******************************************************************/

    /* Public Attributes */

    public:


    /******************************************************************/

    /* Private Attributes */

    private:

        /**
         * @brief Static pointer to Singleton Object instance.
         */
        static Manager_MQTT* instance;

    /******************************************************************/
};

/*****************************************************************************/

/* Include Guards Close */

#endif /* MANAGER_MQTT_H */
