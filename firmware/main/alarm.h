// alarm.h - latched alarm: once tripped, it stays on until it is reset.
#pragma once

#include <stdbool.h>

// Set up the alarm LED and start with the alarm off.
void alarm_init(void);

// Turn the alarm on (LED on) and print the reason. It stays latched.
void alarm_trip(const char *reason);

// Turn the alarm off. Returns true if an alarm was active.
// If the fault is still present, the next check trips it again.
bool alarm_reset(void);

// True while the alarm is latched.
bool alarm_is_active(void);
