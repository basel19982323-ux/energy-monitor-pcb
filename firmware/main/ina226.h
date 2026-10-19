// ina226.h - driver for the TI INA226 voltage/current/power monitor (I2C).
#pragma once

#include <stdbool.h>

// One measurement, already converted to real units.
typedef struct {
    float volts;   // bus (panel) voltage, V
    float amps;    // current through the shunt, A (negative = reverse flow)
    float watts;   // power, W
} ina226_reading_t;

// Start the I2C bus, check the manufacturer ID and write the calibration.
// Returns true if a TI INA226 answered.
bool ina226_init(void);

// Read voltage, current and power into *out.
// Returns false if the sensor did not answer (out is then left unchanged).
bool ina226_read(ina226_reading_t *out);
