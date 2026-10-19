// ina226.c - driver for the TI INA226 voltage/current/power monitor (I2C).
#include "ina226.h"

#include <stdio.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "config.h"

// Register addresses (INA226 datasheet, register map)
#define REG_BUS_VOLTAGE      0x02
#define REG_POWER            0x03
#define REG_CURRENT          0x04
#define REG_CALIBRATION      0x05
#define REG_MANUFACTURER_ID  0xFE

#define TI_MANUFACTURER_ID   0x5449             // "TI"
#define BUS_LSB_V            0.00125f           // fixed by the chip: 1.25 mV per bit
#define POWER_LSB_W          (25.0f * CURRENT_LSB_A)  // datasheet: power LSB = 25 x current LSB
#define I2C_SPEED_HZ         100000             // standard mode
#define I2C_TIMEOUT_MS       100

static i2c_master_dev_handle_t s_dev;           // handle to the sensor on the bus
static bool s_calibrated = false;               // false = calibration must be (re)written

// Read one 16-bit register (the chip sends the high byte first).
static bool read_reg(uint8_t reg, uint16_t *value)
{
    uint8_t data[2] = {0, 0};
    esp_err_t err = i2c_master_transmit_receive(s_dev, &reg, 1, data, sizeof(data), I2C_TIMEOUT_MS);
    *value = ((uint16_t)data[0] << 8) | data[1];
    return err == ESP_OK;
}

// Write one 16-bit register: register number, then high byte, then low byte.
static bool write_reg(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = { reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), I2C_TIMEOUT_MS) == ESP_OK;
}

bool ina226_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port   = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,   // the PCB also has 4.7k pull-ups
    };
    i2c_master_bus_handle_t bus;
    if (i2c_new_master_bus(&bus_config, &bus) != ESP_OK) {
        printf("[sensor] I2C bus could not start\n");
        return false;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = INA226_ADDR,
        .scl_speed_hz    = I2C_SPEED_HZ,
    };
    if (i2c_master_bus_add_device(bus, &dev_config, &s_dev) != ESP_OK) {
        printf("[sensor] could not add the INA226 to the bus\n");
        return false;
    }

    uint16_t id = 0;
    bool found = read_reg(REG_MANUFACTURER_ID, &id) && id == TI_MANUFACTURER_ID;
    printf("[sensor] ID = 0x%04X %s\n", id, found ? "(TI INA226 found)" : "(NOT FOUND!)");

    // Without calibration the INA226 reports current and power as 0.
    s_calibrated = write_reg(REG_CALIBRATION, INA226_CAL);
    return found;
}

bool ina226_read(ina226_reading_t *out)
{
    uint16_t bus = 0, current = 0, power = 0;
    bool ok = read_reg(REG_BUS_VOLTAGE, &bus)
           && read_reg(REG_CURRENT, &current)
           && read_reg(REG_POWER, &power);

    if (!ok) {
        // The sensor may have lost power: a restarted INA226 forgets its calibration.
        s_calibrated = false;
        return false;
    }
    if (!s_calibrated) {
        // Sensor is back: write the calibration again (valid from the next reading).
        s_calibrated = write_reg(REG_CALIBRATION, INA226_CAL);
    }

    out->volts = bus * BUS_LSB_V;
    out->amps  = (int16_t)current * CURRENT_LSB_A;   // current can be negative
    out->watts = power * POWER_LSB_W;
    return true;
}
