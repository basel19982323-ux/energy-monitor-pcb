// config.h - every setting of the firmware in one place.
// Values match the Solar Energy Monitor PCB (rev v0.2): ESP32-C3 + INA226 + 20 mOhm shunt.
#pragma once

#include "driver/gpio.h"

// ---------- Pins (see hardware/energy-monitor.kicad_sch) ----------
#define LED_PIN        GPIO_NUM_3   // alarm LED (free pin; LED planned for PCB rev 0.3)
#define BUTTON_PIN     GPIO_NUM_9   // BOOT button SW2: active-low, 10k pull-up R5
#define I2C_SDA_PIN    GPIO_NUM_0   // IO0 -> INA226 SDA (4.7k pull-up R8)
#define I2C_SCL_PIN    GPIO_NUM_1   // IO1 -> INA226 SCL (4.7k pull-up R4)

// ---------- INA226 current/voltage sensor ----------
#define INA226_ADDR    0x40         // A0 = A1 = GND
#define SHUNT_OHMS     0.020f       // R3, 20 mOhm
#define CURRENT_LSB_A  0.000125f    // full scale 81.92 mV / 0.020 ohm = 4.096 A; / 32768 = 125 uA per bit
#define INA226_CAL     2048         // datasheet: CAL = 0.00512 / (CURRENT_LSB_A x SHUNT_OHMS)

// ---------- Alarm limits ----------
#define OVERCURRENT_LIMIT_A  3.5f   // measurable range ends at 4.096 A: warn before it
#define OVERVOLTAGE_LIMIT_V  22.0f  // board rated 24 V: warn before it

// ---------- Timing ----------
#define SENSOR_PERIOD_MS  1000      // read, check and publish once per second
#define BUTTON_POLL_MS    10        // check the button 100 times per second
#define DEBOUNCE_MS       30        // contact bounce settles well within 30 ms

// ---------- Network ----------
// Defaults are for the Wokwi simulator. On real hardware, set your own network
// (and WIFI_CHANNEL 0 = scan all channels). Do not commit real passwords.
#define WIFI_SSID        "Wokwi-GUEST"
#define WIFI_PASSWORD    ""
#define WIFI_CHANNEL     6
#define MQTT_BROKER_URI  "mqtt://broker.hivemq.com"        // public test broker: never send private data
#define MQTT_TOPIC       "basel-energy-monitor/readings"
