// config.h - Ojo del Sol Calibrator pin and logging settings
#ifndef CALIBRATOR_CONFIG_H
#define CALIBRATOR_CONFIG_H

#include <Arduino.h>

// =============================================================================
// BOARD
// =============================================================================
// Written for an ESP32-S3-CAM board (16MB flash) with the camera NOT used.
// The pins below avoid PSRAM (35-37), the strapping pins (0, 3, 45, 46), native
// USB (19, 20) and UART0 (43, 44, which feed the TTL USB-C port). GPIO39-42 are
// the JTAG pins and GPIO39/40 are also the SD card's CLK/DATA, so leave the SD
// slot empty and don't use JTAG debugging.
//
// Any other ESP32-S3 board works if you change the pins here. Run the cartridge
// bus on pins that are not used by flash/PSRAM on your module.

// =============================================================================
// GBA CARTRIDGE BUS
// =============================================================================
// The cartridge's solar sensor is read through its GPIO port at 0x080000C4-C8
// using normal ROM-bus read/write cycles. Only the address/data lines that this
// needs are driven by the ESP32 (halfword addresses 0x62-0x64, values 0-15):
//
//   AD0, AD1, AD2, AD3, AD5, AD6  -> the six GPIOs in CART_AD_PINS (in that order)
//   /CS, /RD, /WR                 -> CART_CS_PIN, CART_RD_PIN, CART_WR_PIN
//
// Required external wiring (see the README):
//   AD4 and AD7-AD15 : each through ~10k to GND
//   A16-A23          : GND
//   /CS2 (SRAM)      : 3V3 through ~10k (deselects the save RAM)
//   /CS, /RD, /WR    : ~10k pull-up to 3V3 each (keeps them idle while the ESP32 boots)
//   VCC / GND        : 3V3 / GND.  CLK and /IRQ: leave unconnected.
const uint8_t CART_AD_PINS[6] = { 1, 2, 14, 21, 47, 42 };  // AD0, AD1, AD2, AD3, AD5, AD6
const uint8_t CART_CS_PIN = 41;
const uint8_t CART_RD_PIN = 40;
const uint8_t CART_WR_PIN = 39;

// Time per sensor "tick". The original game clocks the sensor from a timer that
// fires every 0x3128 CPU cycles (about 750 microseconds). Two ticks make one
// clock pulse. Raise this if the sensor reads look unstable.
const uint32_t CART_TICK_US = 750;

// =============================================================================
// LTR390 UV SENSOR (I2C)
// =============================================================================
// Same settings as the main Ojo del Sol firmware: UV mode, 18x gain, 20-bit
// (~400 ms) integration, 500 ms measurement rate, 2300 counts per UVI.
// GPIO4/5 are the camera's own control bus; they are used here for the LTR390
// (the OV2640 answers at 0x30 and the LTR390 at 0x53, so they do not collide).
const int I2C_SDA_PIN = 4;
const int I2C_SCL_PIN = 5;
const float UV_SENSITIVITY_COUNTS_PER_UVI = 2300.0f;

// =============================================================================
// SAMPLING
// =============================================================================
// One row is logged per sample. Each sample is lined up with the LTR390's
// 500 ms measurement period and the cartridge read (~0.35 s in the dark) happens
// inside it, so a sample takes about 1000 ms and the interval can't usefully be
// shorter. Use a longer interval for longer runs. If a sample takes longer than
// the interval, the next one starts immediately.
const uint32_t SAMPLE_INTERVAL_MS = 1000;

// =============================================================================
// STORAGE (LittleFS in flash)
// =============================================================================
// Each power-up starts a new file /cal_NNNN.csv. Data is committed to flash
// every FLUSH_EVERY_N_SAMPLES rows, so at most that many rows are lost on a
// sudden power cut. Logging stops (and the LED turns red) when free space drops
// below MIN_FREE_BYTES. Roughly 60 bytes per row: 1 sample/s is ~215 KB per hour.
// The Arduino partition scheme must include a SPIFFS (LittleFS) data partition:
//   "No OTA (1MB APP/3MB SPIFFS)"                  ~3 MB = about 14 hours at 1 sample/s
//   "ESP SR 16M (3MB APP/6MB SPIFFS/3.9MB MODEL)"  ~6 MB = about 28 hours (16MB flash only)
// For longer runs, raise SAMPLE_INTERVAL_MS (5000 ms stretches 3 MB to ~3 days).
const uint32_t FLUSH_EVERY_N_SAMPLES = 10;
const uint32_t MIN_FREE_BYTES = 65536;

// =============================================================================
// SERIAL / STATUS LED
// =============================================================================
// Serial uses UART0 (the "TTL" USB-C port on the ESP32-S3-CAM) at this baud rate.
const uint32_t SERIAL_BAUD = 115200;
// Print every logged row to Serial. Can be toggled at runtime with "live on/off".
const bool LIVE_PRINT_DEFAULT = true;

// On-board WS2812 LED (GPIO48 on the ESP32-S3-CAM). Set to -1 to disable.
// Green = logging fine, yellow = last sample had a cartridge/LTR390 problem,
// red = storage problem.
const int STATUS_LED_PIN = 48;
const uint8_t STATUS_LED_BRIGHTNESS = 8;  // 0-255

#endif
