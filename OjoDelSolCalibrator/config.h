// config.h - Ojo del Sol Calibrator pin and logging settings
#ifndef CALIBRATOR_CONFIG_H
#define CALIBRATOR_CONFIG_H

#include <Arduino.h>

// =============================================================================
// BOARD
// =============================================================================
// Written for an ESP32-S3-CAM board (16MB flash) with the camera NOT used.
// The pins below avoid octal PSRAM (33-37), the strapping pins (0, 3, 45, 46), native
// USB (19, 20) and UART0 (43, 44, which feed the TTL USB-C port). GPIO39-42 are
// the JTAG pins and GPIO38/39/40 are also the SD card's CMD/CLK/DATA, so leave the SD
// slot empty and don't use JTAG debugging. The whole cartridge bus sits on the board's
// right-hand header (GPIO1, 2, 21, 38-42, 47); only the LTR390's SDA/SCL (GPIO4/5) are on
// the left header, next to 3V3 (I2C could move to the right header too, but the free pins
// there are the on-board LED GPIO48 and the strapping/UART pins, so it stays on 4/5).
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
//   AD4 and AD7-AD15 : each through ~10k to GND (must be low when the address latches;
//                      a direct tie also works since the cartridge drives zeros there,
//                      the resistors just guard against it ever driving a 1)
//   A16-A23          : GND
//   /CS2 (SRAM)      : 3V3, wired directly (deselects the save RAM; it is an input)
//   /CS, /RD, /WR    : optional ~10k pull-up to 3V3 each (only keeps them idle while
//                      the ESP32 boots)
//   VCC / GND        : 3V3 / GND.  CLK and /IRQ: leave unconnected.
//
// Pin order: the usable pins on the board's right-hand header, bottom to top (USB-C ports at
// the bottom), are 21, 47, 38, 39, 40, 41, 42, 2, 1. The cartridge pins in order are /WR, /RD,
// /CS, AD0, AD1, AD2, AD3, AD5, AD6, so mapping them onto that sequence needs no wire
// crossings. This direction also keeps the active-low control lines off GPIO1-14, which give
// a ~60 us low pulse at power-up (datasheet Table 2-2); only address/data lines sit on GPIO1/2.
const uint8_t CART_AD_PINS[6] = { 39, 40, 41, 42, 2, 1 };  // AD0, AD1, AD2, AD3, AD5, AD6
const uint8_t CART_CS_PIN = 38;
const uint8_t CART_RD_PIN = 47;
const uint8_t CART_WR_PIN = 21;

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
// The default of 1000 ms samples back to back. For runs of several days use a longer
// interval (it can also be changed from the web app or serial); see the storage notes below.
const uint32_t SAMPLE_INTERVAL_MS = 1000;

// =============================================================================
// STORAGE (LittleFS in flash)
// =============================================================================
// Each power-up starts a new file /cal_NNNN.csv. Data is committed to flash
// every FLUSH_EVERY_N_SAMPLES rows, so at most that many rows are lost on a
// sudden power cut. Logging stops (and the LED turns red) when free space drops
// below MIN_FREE_BYTES; deleting sessions (web app or serial) resumes logging in a new session.
// A row is 33-43 bytes, so one sample per second is roughly 120-155 KB per hour.
// All sessions on the device share the space, so download and delete earlier runs first.
// The partitions.csv next to this sketch (for a 16 MB module) gives the app 3 MB, because the
// Wi-Fi web app makes the firmware ~1 MB, and leaves ~12.9 MB of LittleFS ("spiffs") for the
// logs: roughly 4 days at 1 s (about 8 days at 2 s). On an 8 MB module, edit or delete
// partitions.csv and pick a scheme with a SPIFFS partition (e.g. "8M with spiffs": 1.5 MB,
// about half a day at 1 s).
// For longer runs, raise SAMPLE_INTERVAL_MS. These are estimates from the row size, not
// measured on a device.
const uint32_t FLUSH_EVERY_N_SAMPLES = 10;
const uint32_t MIN_FREE_BYTES = 65536;

// =============================================================================
// WI-FI / WEB APP
// =============================================================================
// The web app shows live readings, browses past sessions and manages and backs up the
// stored logs. Open it at http://ojodelsol-calibrator.local (MDNS_HOSTNAME), or by IP.
//
// Your Wi-Fi: the calibrator joins the network in logins.h (copy logins.h.example to
// logins.h; git ignores it). It uses modem sleep there, so the radio naps between the
// router's beacons; that should draw much less than running a hotspot (not measured).
// The IP address is printed on serial at boot. If the connection drops later, it
// keeps trying to reconnect.
//
// Hotspot: if there is no logins.h, or joining fails within WIFI_STA_CONNECT_TIMEOUT_MS,
// it creates its own network instead (no internet or router needed). Join it from a
// phone or laptop and open http://192.168.4.1 (a captive-portal prompt should also pop
// up). CHANGE THE HOTSPOT PASSWORD: anyone in range who knows it can read and DELETE
// your logs. It must be 8-63 characters; an empty string makes an open network (not
// recommended). The hotspot keeps the radio on (roughly 100 mA or more; not measured).
//
// Wi-Fi interrupts could, in principle, jitter the cartridge timing; set
// WIFI_ENABLED = false to run without Wi-Fi at all (serial commands still work).
const bool WIFI_ENABLED = true;

#if __has_include("logins.h")
#include "logins.h"
#endif
#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
const char WIFI_STA_SSID[] = WIFI_SSID;
const char WIFI_STA_PASSWORD[] = WIFI_PASSWORD;
#else
const char WIFI_STA_SSID[] = "";       // No logins.h: hotspot only
const char WIFI_STA_PASSWORD[] = "";
#endif
const uint32_t WIFI_STA_CONNECT_TIMEOUT_MS = 20000;
const char MDNS_HOSTNAME[] = "ojodelsol-calibrator";  // -> http://ojodelsol-calibrator.local

// Internet time (only on your Wi-Fi). The device has no clock, so once connected it
// asks these NTP servers for the time, trying the next one if a server doesn't answer,
// and re-syncs every 3 hours. Timestamps are stored in UTC; the web app shows them in
// the viewing device's own time zone, so no time zone is set here. On the hotspot (no
// internet), or until a server answers, the phone that opens the web app sets the clock.
const char NTP_SERVER_1[] = "pool.ntp.org";
const char NTP_SERVER_2[] = "time.google.com";       // Fallbacks, from different operators
const char NTP_SERVER_3[] = "time.cloudflare.com";

const char WIFI_AP_SSID[] = "OjoDelSol-Calibrator";
const char WIFI_AP_PASSWORD[] = "ojodelsol";
const uint8_t WIFI_AP_CHANNEL = 6;
const uint8_t WIFI_AP_MAX_CLIENTS = 4;
const bool WIFI_AP_HIDDEN = false;

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
