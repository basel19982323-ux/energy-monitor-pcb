// Solar Energy Monitor firmware - ESP32-C3 + INA226
// Reads panel voltage, current and power every second, raises a latched alarm on
// overcurrent, overvoltage or a missing sensor, and publishes readings over MQTT.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "config.h"
#include "ina226.h"
#include "alarm.h"
#include "network.h"

// Task 1: the BOOT button resets the alarm (polled, with debounce).
static void button_task(void *arg)
{
    int last_level = 1;                                   // 1 = released (pull-up)
    while (1) {
        int level = gpio_get_level(BUTTON_PIN);
        if (last_level == 1 && level == 0) {              // press just started
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            if (gpio_get_level(BUTTON_PIN) == 0) {        // still pressed: a real press
                printf("%s\n", alarm_reset() ? "[button] alarm reset" : "[button] no alarm to reset");
            }
        }
        last_level = level;
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}

// Compare one reading with the limits and trip the alarm if needed.
static void check_limits(const ina226_reading_t *r)
{
    char reason[64];
    if (r->amps > OVERCURRENT_LIMIT_A) {
        snprintf(reason, sizeof(reason), "overcurrent: %.2f A (limit %.2f A)", r->amps, OVERCURRENT_LIMIT_A);
        alarm_trip(reason);
    }
    if (r->volts > OVERVOLTAGE_LIMIT_V) {
        snprintf(reason, sizeof(reason), "overvoltage: %.2f V (limit %.2f V)", r->volts, OVERVOLTAGE_LIMIT_V);
        alarm_trip(reason);
    }
}

// Task 2: every second read the sensor, print, check limits and publish.
static void sensor_task(void *arg)
{
    while (1) {
        ina226_reading_t r;
        if (!ina226_read(&r)) {
            alarm_trip("sensor not responding - check the I2C wires");
            network_publish("{\"error\":\"sensor not responding\"}");
        } else {
            printf("[sensor] %.2f V   %.3f A   %.2f W%s\n",
                   r.volts, r.amps, r.watts, alarm_is_active() ? "   (ALARM ON)" : "");
            check_limits(&r);

            char message[128];
            snprintf(message, sizeof(message),
                     "{\"volts\":%.2f,\"amps\":%.3f,\"watts\":%.2f,\"alarm\":%s}",
                     r.volts, r.amps, r.watts, alarm_is_active() ? "true" : "false");
            network_publish(message);
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}

void app_main(void)
{
    alarm_init();

    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);    // matches R5 on the PCB

    ina226_init();
    network_start();

    xTaskCreate(button_task, "button", 2048, NULL, 5, NULL);
    xTaskCreate(sensor_task, "sensor", 6144, NULL, 5, NULL);   // larger stack: float formatting
}
