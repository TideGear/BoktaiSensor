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
// When true, bar thresholds are auto-calculated from UV_MIN and UV_SATURATION.
// This uses the same UV range for all games, evenly distributing bars.
// Note: UV_SATURATION is a ceiling/clamp point, not the first full-bar threshold.
// Set to false to use the per-game manual calibration arrays below.
const bool AUTO_MODE = true;

const float AUTO_UV_MIN = 0.5;   // UV Index for 1 bar (shade/overcast)
const float AUTO_UV_SATURATION = 6.0;   // UV Index where output is fully saturated/clamped

// -----------------------------------------------------------------------------
// UV ENCLOSURE COMPENSATION
// -----------------------------------------------------------------------------
// Model an enclosure window as UV transmission loss and compensate once after
// raw-to-UVI conversion:
//   corrected_uvi = max(0, (measured_uvi - UV_ENCLOSURE_UVI_OFFSET) /
//                           UV_ENCLOSURE_TRANSMITTANCE)
// Keep UV_ENCLOSURE_TRANSMITTANCE in the 0.0-1.0 range.
// Example: 0.42 means the window passes 42% of UV.
const bool UV_ENCLOSURE_COMP_ENABLED = false;
const float UV_ENCLOSURE_TRANSMITTANCE = 1.0f;
const float UV_ENCLOSURE_UVI_OFFSET = 0.0f;

// -----------------------------------------------------------------------------
// UV FILTERING
// -----------------------------------------------------------------------------
// Exponential smoothing reduces jitter in the UV bar display.
// UVI_SMOOTHING_ALPHA controls how much weight new readings get vs. the running average.
// Setting UVI_SMOOTHING_ALPHA = 1.0 is the same as UVI_SMOOTHING_ENABLED = false.
// Hysteresis adds a small UV margin before changing bars.
// Setting BAR_HYSTERESIS_ENABLED = false is the same as BAR_HYSTERESIS = 0.0.
const bool UVI_SMOOTHING_ENABLED = false;
const float UVI_SMOOTHING_ALPHA = 0.5;  // 0.0-1.0. Controls how much weight new readings get vs. the running average. Higher = faster response.
const bool BAR_HYSTERESIS_ENABLED = false;
const float BAR_HYSTERESIS = 0.2;       // UV Index margin for bar changes

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

// =============================================================================
// POWER MANAGEMENT
// =============================================================================

// -----------------------------------------------------------------------------
// BATTERY MONITORING
// -----------------------------------------------------------------------------
// NOTE: The XIAO ESP32S3 does NOT have built-in battery voltage monitoring!
// You must add a voltage divider (2x 100K resistors) from BAT+ to GND with
// the midpoint connected to BAT_PIN. Without this, battery % is unavailable.
// Set BATTERY_SENSE_ENABLED = false if you do not wire the divider.
const bool BATTERY_SENSE_ENABLED = true;
const int BAT_PIN = 2;       // D1/A1 (GPIO2) - connect to voltage divider midpoint
const float VOLT_MIN = 3.3;  // 0% Battery
const float VOLT_MAX = 4.2;  // 100% Battery
const unsigned long BATTERY_SAMPLE_MS = 1000;  // Battery update interval

// Voltage divider calibration multiplier
// Theoretical: 2.0 for equal resistors (100K + 100K = 2:1 ratio)
// Actual default: 2.21 compensates for typical ADC/resistor variance
// Adjust if battery % is wrong at full charge:
//   - If % too LOW:  increase this value
//   - If % too HIGH: decrease this value
const float VOLT_DIVIDER_MULT = 2.21;

// -----------------------------------------------------------------------------
// LOW-VOLTAGE CUTOFF (requires battery sense divider)
// -----------------------------------------------------------------------------
const bool BATTERY_CUTOFF_ENABLED = true;
const float VOLT_CUTOFF = VOLT_MIN;  // Cutoff threshold under load
const unsigned long BATTERY_CUTOFF_HOLD_MS = 5000;

// -----------------------------------------------------------------------------
// SENSOR POWER CONTROL
// -----------------------------------------------------------------------------
// Optionally power the LTR390 from a GPIO to cut power during sleep.
// Use a GPIO capable of sourcing a few mA and never power the OLED this way.
// Default: enabled; wire LTR390 VIN to SENSOR_POWER_PIN (D3/GPIO4).
// Set to false if powering the sensor from 3V3.
const bool SENSOR_POWER_ENABLED = true;
const int SENSOR_POWER_PIN = 4;               // D3 (GPIO4)
const unsigned long SENSOR_POWER_STABLE_MS = 10;

// =============================================================================
// USER INTERFACE
// =============================================================================

// -----------------------------------------------------------------------------
// POWER BUTTON
// -----------------------------------------------------------------------------
// D0 (GPIO1) supports RTC wake-up from deep sleep on XIAO ESP32S3.
const int BUTTON_PIN = 1;    // D0 (GPIO1)
const unsigned long DEBOUNCE_MS = 50;       // Button debounce time
const unsigned long LONG_PRESS_MS = 3000;   // Hold 3 seconds to power on/off

// -----------------------------------------------------------------------------
// DISPLAY SCREENSAVER
// -----------------------------------------------------------------------------
// After SCREENSAVER_TIME minutes of no button activity, show a bouncing
// "Ojo del Sol" screensaver.
const bool SCREENSAVER_ACTIVE = true;
const unsigned long SCREENSAVER_TIME = 3;  // Minutes; 0 disables (same as false)

// =============================================================================
// CONNECTIVITY
// =============================================================================

// -----------------------------------------------------------------------------
// GBA LINK OUTPUT (D7-D10)
// -----------------------------------------------------------------------------
const bool GBA_LINK_ENABLED = true;
const int GBA_PIN_SC = 44;  // D7 (GPIO44) - SC (bit 0)
const int GBA_PIN_SD = 7;   // D8 (GPIO7)  - SD (bit 1)
const int GBA_PIN_SI = 8;   // D9 (GPIO8)  - SI (bit 2)
const int GBA_PIN_SO = 10;  // D10 (GPIO10) - SO (bit 3)

// -----------------------------------------------------------------------------
// BLUETOOTH HID
// -----------------------------------------------------------------------------
// Set BLUETOOTH_ENABLED to false to disable BLE entirely.
const bool BLUETOOTH_ENABLED = true;
const char BLE_DEVICE_NAME[] = "Ojo del Sol Sensor";
const unsigned long BLE_PAIRING_TIMEOUT_MS = 60000; // Stop pairing (or re-pairing after
// dropping connection) after 1 minute
const bool BLE_RESYNC_ENABLED = true;
const unsigned long BLE_RESYNC_INTERVAL_MS = 60000; // Clamp + refill interval
const unsigned int BLE_BUTTONS_PER_SECOND = 20;     // Button press rate
// Workaround: mGBA uses 10 steps for Boktai 1 even though it has 8 bars.
// Set false if mGBA is fixed to use 8 steps.
const bool BLE_BOKTAI1_MGBA_10_STEP_WORKAROUND = true;
// BLE control mode:
// 0 = Incremental (default): uses BLE_BUTTON_DEC/INC to step the meter; resync enabled.
// 1 = Single Analog: maps bar count to a proportional deflection on one analog axis.
const uint8_t BLE_CONTROL_MODE = 0;

// Incremental mode button mapping (from XboxGamepadDevice.h)
// XBOX_BUTTON_LS = 0x2000 (Left Stick click = L3)
// XBOX_BUTTON_RS = 0x4000 (Right Stick click = R3)
const uint16_t BLE_BUTTON_DEC = 0x2000;              // L3 (Left Stick click)
const uint16_t BLE_BUTTON_INC = 0x4000;              // R3 (Right Stick click)

// Single Analog axis (used when BLE_CONTROL_MODE = 1):
// 0 = Left  X-  (left stick left)
// 1 = Left  X+  (left stick right)
// 2 = Left  Y-  (left stick up)
// 3 = Left  Y+  (left stick down)
// 4 = Right X-  (right stick left)
// 5 = Right X+  (right stick right)  [default]
// 6 = Right Y-  (right stick up)
// 7 = Right Y+  (right stick down)
const uint8_t BLE_SINGLE_ANALOG_AXIS = 5;
// When true, a configurable button is held while the analog axis is deflected.
// With midpoint band mapping, 0 bars still has a small deflection, so this button
// remains held during normal Single Analog updates and is released only when the
// controlled axis is explicitly returned to center.
const bool BLE_METER_UNLOCK_BUTTON_ENABLED = true;
const uint16_t BLE_METER_UNLOCK_BUTTON = 0x4000;  // R3 (Right Stick click)
// How often (ms) to release and re-press the unlock button so that apps/emulators
// that start listening mid-session still register it as held. 0 = no periodic refresh.
const unsigned long BLE_METER_UNLOCK_REFRESH_MS = 1000;

// =============================================================================
// DEBUG
// =============================================================================
// Enables Serial debug logging (115200 baud) for UV/battery readings and sensor init.
const bool DEBUG_SERIAL = true;

// Adds a 4th screen (after Boktai 3) showing raw UV + battery values for tuning.
// Set false to remove it from the button cycle.
const bool DEBUG_SCREEN_ENABLED = true;

#endif
