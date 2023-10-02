# esp_can_sniffer
ESP-IDF project to create a listen only device to log CAN Bus messages.

## Hardware Setup

The ESP32 contains a built-in TWAI Controller peripheral that can be used for CAN communication by adding an external CAN transceiver that provides the required physical level for the Bus. A CJMCU-1051 board is used, it has a TJA1051T CAN transceiver, however this variant used doesn't allow to supply diferent power levels for control logic (no Vio pin available; TJA1051T/3 and TJA1051TK/3 only), so a power level shifter is also required to protect the ESP32 GPIOs.

Here is a basic connection diagram:

![hardware_setup](https://github.com/J-Rios/esp_can_sniffer/assets/12136967/dc264c5a-7996-4574-b69e-a5425ca66f88)

## Usage

Once firmware is flashed in the ESP32 and the CANH and CANL signals are connected to a CAN Bus, open a Serial console at 115200 bauds speed and start logging CAN messages and CAN Bus events (as errors).

Log result example:

![imagen](https://github.com/J-Rios/esp_can_sniffer/assets/12136967/5e3b59e7-4123-4fd3-9c45-69f19e0f52b7)

Note: [SUSTerm](https://github.com/J-Rios/SUSTerm) used as Serial console.
