// config.h - Thresholds and Pin Settings
#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// BOKTAI CALIBRATION
// =============================================================================
// Original cart sensor: 0xE8 = darkness, 0x50 = max gauge, 0x00 = extreme
// LTR390 UV Index: 0 = darkness, 11+ = extreme UV
//
// Boktai 1: 8-bar sun gauge
// Boktai 2 & 3: 10-bar sun gauge
//
// Tune these values by comparing against original cartridges.
// =============================================================================

// -----------------------------------------------------------------------------
// AUTOMATIC MODE (Recommended)
// -----------------------------------------------------------------------------
// When true, bar thresholds are auto-calculated from UV_MIN and UV_MAX.
// This uses the same UV range for all games, evenly distributing bars.
// Set to false to use the per-game manual calibration arrays below.
const bool AUTO_MODE = true;

const float AUTO_UV_MIN = 0.5;   // UV Index for 1 bar (shade/overcast)
const float AUTO_UV_MAX = 6.0;   // UV Index for full bars (typical sunny day)

// -----------------------------------------------------------------------------
// GAME DEFINITIONS
// -----------------------------------------------------------------------------
// Number of supported games
const int NUM_GAMES = 3;

// Game names (shown on display)
const char* GAME_NAMES[NUM_GAMES] = {
  "BOKTAI 1",
  "BOKTAI 2", 
  "BOKTAI 3"
};

// Bar counts per game
const int GAME_BARS[NUM_GAMES] = { 8, 10, 10 };

// -----------------------------------------------------------------------------
// MANUAL MODE CALIBRATION (only used when AUTO_MODE = false)
// -----------------------------------------------------------------------------
// BOKTAI 1 - 8 bars (range: 0.5 to 6.0)
const float BOKTAI_1_UV[8] = {
  0.5,   // Bar 1: Minimal UV (shade/overcast)
  1.3,   // Bar 2: Low UV
  2.1,   // Bar 3: Low-moderate UV
  2.9,   // Bar 4: Moderate UV
  3.6,   // Bar 5: Moderate-high UV
  4.4,   // Bar 6: High UV (partly sunny)
  5.2,   // Bar 7: High UV (sunny)
  6.0    // Bar 8: Full sun
};

// BOKTAI 2 - 10 bars (range: 0.5 to 6.0)
const float BOKTAI_2_UV[10] = {
  0.5,   // Bar 1
  1.1,   // Bar 2
  1.7,   // Bar 3
  2.3,   // Bar 4
  2.9,   // Bar 5
  3.5,   // Bar 6
  4.1,   // Bar 7
  4.7,   // Bar 8
  5.3,   // Bar 9
  6.0    // Bar 10
};

// BOKTAI 3 - 10 bars (range: 0.5 to 6.0)
const float BOKTAI_3_UV[10] = {
  0.5,   // Bar 1
  1.1,   // Bar 2
  1.7,   // Bar 3
  2.3,   // Bar 4
  2.9,   // Bar 5
  3.5,   // Bar 6
  4.1,   // Bar 7
  4.7,   // Bar 8
  5.3,   // Bar 9
  6.0    // Bar 10
};

// -----------------------------------------------------------------------------
// UV FILTERING (Optional)
// -----------------------------------------------------------------------------
// Exponential smoothing reduces jitter in the UV bar display.
// Hysteresis adds a small UV margin before changing bars.
const bool UVI_SMOOTHING_ENABLED = true;
const float UVI_SMOOTHING_ALPHA = 0.2;  // 0.0-1.0 (higher = faster response)
const float BAR_HYSTERESIS = 0.2;       // UV Index margin for bar changes

// Battery Monitoring
// NOTE: The XIAO ESP32S3 does NOT have built-in battery voltage monitoring!
// You must add a voltage divider (2x 100K resistors) from BAT+ to GND with
// the midpoint connected to BAT_PIN. Without this, battery % is unavailable.
// Set BATTERY_SENSE_ENABLED = false if you do not wire the divider.
const bool BATTERY_SENSE_ENABLED = true;
const int BAT_PIN = 2;       // D1/A1 (GPIO2) - connect to voltage divider midpoint
const float VOLT_MIN = 3.3;  // 0% Battery
const float VOLT_MAX = 4.2;  // 100% Battery
const unsigned long BATTERY_SAMPLE_MS = 1000;  // Battery update interval

// Low-voltage cutoff (requires battery sense divider)
const bool BATTERY_CUTOFF_ENABLED = true;
const float VOLT_CUTOFF = VOLT_MIN;  // Cutoff threshold under load
const unsigned long BATTERY_CUTOFF_HOLD_MS = 5000;

// Voltage divider calibration multiplier
// Theoretical: 2.0 for equal resistors (100K + 100K = 2:1 ratio)
// Actual default: 2.25 compensates for typical ADC/resistor variance
// Adjust if battery % is wrong at full charge:
//   - If % too LOW:  increase this value
//   - If % too HIGH: decrease this value
const float VOLT_DIVIDER_MULT = 2.25;

// Sensor Power Control
// Optionally power the LTR390 from a GPIO to cut power during sleep.
// Use a GPIO capable of sourcing a few mA and never power the OLED this way.
// Default: enabled; wire LTR390 VIN to SENSOR_POWER_PIN (D3/GPIO4).
// Set to false if powering the sensor from 3V3.
const bool SENSOR_POWER_ENABLED = true;
const int SENSOR_POWER_PIN = 4;               // D3 (GPIO4)
const unsigned long SENSOR_POWER_STABLE_MS = 10;

// Power Button Settings
// D0 (GPIO1) supports RTC wake-up from deep sleep on XIAO ESP32S3.
const int BUTTON_PIN = 1;    // D0 (GPIO1)
const unsigned long DEBOUNCE_MS = 50;       // Button debounce time
const unsigned long LONG_PRESS_MS = 3000;   // Hold 3 seconds to power on/off

// Display Screensaver
// After SCREENSAVER_TIME minutes of no button activity, show a bouncing
// "Ojo del Sol" screensaver. Set SCREENSAVER_TIME to 0 to disable.
const bool SCREENSAVER_ACTIVE = true;
const unsigned long SCREENSAVER_TIME = 3;  // Minutes; 0 disables (same as false)

// GBA Link output (D7-D10)
const bool GBA_LINK_ENABLED = true;
const int GBA_PIN_SC = 44;  // D7 (GPIO44) - SC (bit 0)
const int GBA_PIN_SD = 7;   // D8 (GPIO7)  - SD (bit 1)
const int GBA_PIN_SI = 8;   // D9 (GPIO8)  - SI (bit 2)
const int GBA_PIN_SO = 10;  // D10 (GPIO10) - SO (bit 3)

// Debug logging
const bool DEBUG_SERIAL = false;

#endif
