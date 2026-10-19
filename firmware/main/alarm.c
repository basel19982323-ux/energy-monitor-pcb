// alarm.c - latched alarm: once tripped, it stays on until it is reset.
#include "alarm.h"

#include <stdio.h>
#include "driver/gpio.h"
#include "config.h"

// Shared by the sensor task (trips it) and the button task (resets it).
// volatile: always read the real value, another task may have changed it.
static volatile bool s_latched = false;

void alarm_init(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);
    s_latched = false;
}

void alarm_trip(const char *reason)
{
    printf("[ALARM] %s\n", reason);
    s_latched = true;
    gpio_set_level(LED_PIN, 1);
}

bool alarm_reset(void)
{
    bool was_active = s_latched;
    s_latched = false;
    gpio_set_level(LED_PIN, 0);
    return was_active;
}

bool alarm_is_active(void)
{
    return s_latched;
}
