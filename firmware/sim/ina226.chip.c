// Pretend INA226 for Wokwi: answers on I2C exactly like the real TI chip
#include "wokwi-api.h"
#include <stdio.h>
#include <stdint.h>

#define R_SHUNT 0.02f            // 20 mOhm, same as R3 on the PCB

typedef struct {                 // all the chip's boxes, grouped together
  uint32_t voltage_slider;       // handle to the "Panel voltage" slider
  uint32_t current_slider;       // handle to the "Current" slider
  uint8_t  drawer;               // which drawer (register) the ESP32 picked
  uint8_t  byte_count;           // counts bytes in the current message
  uint16_t incoming;             // collects a 2-byte value the ESP32 writes
  uint16_t calibration;          // drawer 0x05 (starts at 0, like the real chip)
} chip_state_t;

static chip_state_t chip;

// Work out what is inside a drawer right now, from the sliders
static uint16_t drawer_value(uint8_t drawer) {
  float volts = attr_read_float(chip.voltage_slider);
  float amps  = attr_read_float(chip.current_slider);
  int32_t shunt   = (int32_t)(amps * R_SHUNT / 0.0000025f + 0.5f); // 2.5 uV per step
  int32_t bus     = (int32_t)(volts / 0.00125f + 0.5f);            // 1.25 mV per step
  int32_t current = shunt * chip.calibration / 2048;               // datasheet formula
  int32_t power   = current * bus / 20000;                         // datasheet formula
  switch (drawer) {
    case 0x01: return (uint16_t)shunt;
    case 0x02: return (uint16_t)bus;
    case 0x03: return (uint16_t)power;
    case 0x04: return (uint16_t)current;
    case 0x05: return chip.calibration;
    case 0xFE: return 0x5449;    // manufacturer ID "TI"
    case 0xFF: return 0x2260;    // INA226 die ID
    default:   return 0;
  }
}

// The ESP32 called our name (0x40): say "yes, I'm here" and start counting bytes
static bool on_connect(void *user_data, uint32_t address, bool read) {
  chip.byte_count = 0;
  return true;
}

// The ESP32 sent us a byte
static bool on_write(void *user_data, uint8_t data) {
  if (chip.byte_count == 0) {
    chip.drawer = data;                      // 1st byte = which drawer
  } else if (chip.byte_count == 1) {
    chip.incoming = data << 8;               // 2nd byte = big part
  } else if (chip.byte_count == 2) {
    chip.incoming = chip.incoming | data;    // 3rd byte = small part
    if (chip.drawer == 0x05) chip.calibration = chip.incoming;
  }
  chip.byte_count++;
  return true;
}

// The ESP32 wants a byte back: big part first, then small part
static uint8_t on_read(void *user_data) {
  uint16_t value = drawer_value(chip.drawer);
  uint8_t out = (chip.byte_count == 0) ? (value >> 8) : (value & 0xFF);
  chip.byte_count++;
  return out;
}

// Runs once when the simulation starts
void chip_init(void) {
  chip.calibration    = 0;
  chip.voltage_slider = attr_init_float("busVoltage", 17.5);
  chip.current_slider = attr_init_float("current", 2.0);

  const i2c_config_t config = {
    .address    = 0x40,
    .scl        = pin_init("SCL", INPUT_PULLUP),
    .sda        = pin_init("SDA", INPUT_PULLUP),
    .connect    = on_connect,
    .read       = on_read,
    .write      = on_write,
    .disconnect = NULL,
    .user_data  = NULL,
  };
  i2c_init(&config);
  printf("INA226 (sim) ready at address 0x40\n");
}