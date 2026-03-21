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
// When true, bar thresholds follow the original Boktai cartridge's non-linear curve
// (per Raphi's sensor graph), scaled between AUTO_UV_MIN and AUTO_UV_SATURATION.
// The same range is used for all games.
// Set to false to use per-game UV_MIN/UV_SATURATION values in the manual section below.
const bool AUTO_MODE = true;

const float AUTO_UV_MIN = 0.500;   // UV Index for 1 bar (shade/overcast)
const float AUTO_UV_SATURATION = 6.000;   // UV Index where output is fully saturated/clamped

// -----------------------------------------------------------------------------
// UV ENCLOSURE COMPENSATION
// -----------------------------------------------------------------------------
// Model an enclosure window as UV transmission loss and compensate once after
// raw-to-UVI conversion:
//   corrected_uvi = max(0, (measured_uvi - UV_ENCLOSURE_UVI_OFFSET) /
//                           UV_ENCLOSURE_TRANSMITTANCE)
// Keep UV_ENCLOSURE_TRANSMITTANCE in the 0.0-1.0 range.
// Example: 0.42 means the window passes 42% of UV.
const bool UV_ENCLOSURE_COMP_ENABLED = true;
// The default setting below is based on a VERY quick and dirty test with my box of choice, open vs closed.
const float UV_ENCLOSURE_TRANSMITTANCE = 0.833f;
const float UV_ENCLOSURE_UVI_OFFSET = 0.000f;

// When true (default), the threshold values in this file (AUTO_UV_MIN, AUTO_UV_SATURATION,
// and the manual calibration arrays) are treated as open-air UVI values — measured with
// the case open or without any enclosure. Bar comparison uses the compensated
// (open-air equivalent) UVI so results remain accurate when the case is closed.
//
// Set false if your threshold values were calibrated with the sensor inside the closed
// enclosure. Bar comparison will then use the raw (pre-compensation) sensor reading.
//
// For always-open-air builds (sensor never enclosed): leave true and set
// UV_ENCLOSURE_COMP_ENABLED = false. Compensation becomes a no-op so raw == compensated.
const bool UV_THRESHOLDS_CALIBRATED_OPEN_AIR = true;

// -----------------------------------------------------------------------------
// UV FILTERING
// -----------------------------------------------------------------------------
// Exponential smoothing reduces jitter in the UV bar display.
// UVI_SMOOTHING_ALPHA controls how much weight new readings get vs. the running average.
// Setting UVI_SMOOTHING_ALPHA = 1.0 is the same as UVI_SMOOTHING_ENABLED = false.
// Hysteresis adds a small UV margin before changing bars.
// Setting BAR_HYSTERESIS_ENABLED = false is the same as BAR_HYSTERESIS = 0.0.
const bool UVI_SMOOTHING_ENABLED = false;
const float UVI_SMOOTHING_ALPHA = 0.5;
const bool BAR_HYSTERESIS_ENABLED = false;
const float BAR_HYSTERESIS = 0.200;       // UV Index margin for bar changes

// -----------------------------------------------------------------------------
// GAME DEFINITIONS
// -----------------------------------------------------------------------------
// Number of supported games
const int NUM_GAMES = 3;

// Game names (shown on display)
const char* const GAME_NAMES[NUM_GAMES] = {
  "BOKTAI 1",
  "BOKTAI 2", 
  "BOKTAI 3"
};

// Bar counts per game
const int GAME_BARS[NUM_GAMES] = { 8, 10, 10 };

// -----------------------------------------------------------------------------
// MANUAL MODE CALIBRATION (only used when AUTO_MODE = false)
// -----------------------------------------------------------------------------
// Per-game UV range overrides. Bar thresholds follow the same non-linear cartridge
// curve as AUTO mode, but each game can use a different UV_MIN/UV_SATURATION.
const float BOKTAI_1_UV_MIN = 0.500;
const float BOKTAI_1_UV_SATURATION = 6.000;

const float BOKTAI_2_UV_MIN = 0.500;
const float BOKTAI_2_UV_SATURATION = 6.000;

// Intentionally separate from BOKTAI_2 for possible future divergence.
const float BOKTAI_3_UV_MIN = 0.500;
const float BOKTAI_3_UV_SATURATION = 6.000;

// =============================================================================
// POWER MANAGEMENT
// =============================================================================

// -----------------------------------------------------------------------------
// BATTERY MONITORING
// -----------------------------------------------------------------------------
// NOTE: The XIAO ESP32S3 does NOT have built-in battery voltage monitoring!
// You must add a voltage divider (2x 100K resistors) from BAT+ to GND with
// the midpoint connected to BAT_PIN. Without this, battery % is unavailable.
// Safe default is disabled until the divider is wired.
const bool BATTERY_SENSE_ENABLED = false;
const int BAT_PIN = 2;       // D1/A1 (GPIO2) - connect to voltage divider midpoint
const float VOLT_MIN = 3.3;  // 0% Battery
const float VOLT_MAX = 4.2;  // 100% Battery (must be > VOLT_MIN)
const unsigned long BATTERY_SAMPLE_MS = 1000;  // Battery update interval

// Voltage divider calibration multiplier
// Theoretical: 2.0 for equal resistors (100K + 100K = 2:1 ratio)
// Actual default: 2.20 compensates for typical ADC/resistor variance
// Adjust if battery % is wrong at full charge:
//   - If % too LOW:  increase this value
//   - If % too HIGH: decrease this value
const float VOLT_DIVIDER_MULT = 2.20;

// -----------------------------------------------------------------------------
// LOW-VOLTAGE CUTOFF (requires battery sense divider)
// -----------------------------------------------------------------------------
// Keep disabled by default; only enable after battery sensing is verified.
const bool BATTERY_CUTOFF_ENABLED = false;
const float VOLT_CUTOFF = VOLT_MIN;  // Cutoff threshold under load
const unsigned long BATTERY_CUTOFF_HOLD_MS = 5000;

// =============================================================================
// USER INTERFACE
// =============================================================================

// -----------------------------------------------------------------------------
// POWER BUTTON
// -----------------------------------------------------------------------------
// D0 (GPIO1) supports RTC wake-up from deep sleep on XIAO ESP32S3.
const int BUTTON_PIN = 1;    // D0 (GPIO1)
const unsigned long DEBOUNCE_MS = 50;       // Button debounce time
const unsigned long LONG_PRESS_MS = 2000;   // Hold 2 seconds to power on/off

// -----------------------------------------------------------------------------
// DISPLAY / I2C
// -----------------------------------------------------------------------------
const int I2C_SDA_PIN = 5;                // D4 (GPIO5)
const int I2C_SCL_PIN = 6;                // D5 (GPIO6)
const uint8_t DISPLAY_I2C_ADDR = 0x3C;   // Some SSD1306 modules use 0x3D

// -----------------------------------------------------------------------------
// DISPLAY SCREENSAVER
// -----------------------------------------------------------------------------
// After SCREENSAVER_TIME minutes of no button activity, show a bouncing
// "Ojo del Sol" screensaver.
const bool SCREENSAVER_ACTIVE = true;
const unsigned long SCREENSAVER_TIME = 3;  // Minutes; 0 disables (equivalent to SCREENSAVER_ACTIVE = false)

// =============================================================================
// CONNECTIVITY
// =============================================================================

// -----------------------------------------------------------------------------
// GBA LINK OUTPUT (framed 3-wire)
// -----------------------------------------------------------------------------
const bool GBA_LINK_ENABLED = true;
// Framed 3-wire protocol:
// - SC = frame phase line
// - SD/SO = 2-bit payload pair
const int GBA_PIN_SC = 44;  // D7 (GPIO44) - frame phase (SC)
const int GBA_PIN_SD = 7;   // D8 (GPIO7)  - payload bit 1 (SD)
const int GBA_PIN_SO = 8;   // D9 (GPIO8)  - payload bit 0 (SO)
// Per-phase hold time. A full 4-bit value takes two phases.
// Example: 5ms => one phase every 5ms, full value refresh about every 10ms.
const unsigned long GBA_LINK_FRAME_TOGGLE_MS = 5;

// -----------------------------------------------------------------------------
// HID CONTROLLER (shared by Bluetooth and USB)
// -----------------------------------------------------------------------------
// Control mode (applies to both Bluetooth HID and USB XInput):
// 0 = Incremental: uses HID_BUTTON_DEC/INC to step the emulator meter.
//     (BLE transport also supports clamp+refill resync behavior.)
// 1 = Single Analog: maps bar count to a proportional deflection on one analog axis.
const uint8_t HID_CONTROL_MODE = 0;

// Workaround: mGBA uses 10 steps for Boktai 1 even though it has 8 bars.
// Set false if mGBA is fixed to use 8 steps.
const bool HID_BOKTAI1_MGBA_10_STEP_WORKAROUND = true;

// Incremental mode button mapping (shared by Bluetooth and USB)
// XBOX_BUTTON_LS = 0x2000 (Left Stick click = L3)
// XBOX_BUTTON_RS = 0x4000 (Right Stick click = R3)
const unsigned int HID_BUTTONS_PER_SECOND = 20;     // Button press rate; 0 disables incremental meter presses
const uint16_t HID_BUTTON_DEC = 0x2000;              // L3 (Left Stick click)
const uint16_t HID_BUTTON_INC = 0x4000;              // R3 (Right Stick click)

// Single Analog axis (shared by Bluetooth and USB; used when HID_CONTROL_MODE = 1):
// 0 = Left  X-  (left stick left)
// 1 = Left  X+  (left stick right)
// 2 = Left  Y-  (left stick up)
// 3 = Left  Y+  (left stick down)
// 4 = Right X-  (right stick left)
// 5 = Right X+  (right stick right)  [default]
// 6 = Right Y-  (right stick up)
// 7 = Right Y+  (right stick down)
const uint8_t HID_SINGLE_ANALOG_AXIS = 5;
// When true, a configurable button is held while the analog axis is deflected.
// With midpoint band mapping, 0 bars still has a small deflection, so this button
// remains held during normal Single Analog updates and is released only when the
// controlled axis is explicitly returned to center.
const bool HID_METER_UNLOCK_BUTTON_ENABLED = true;
const uint16_t HID_METER_UNLOCK_BUTTON = 0x4000;  // R3 (Right Stick click)
// How often (ms) to release and re-press the unlock button so that apps/emulators
// that start listening mid-session still register it as held. 0 = no periodic refresh.
const unsigned long HID_METER_UNLOCK_REFRESH_MS = 1000;

// -----------------------------------------------------------------------------
// BLUETOOTH HID
// -----------------------------------------------------------------------------
// Set BLUETOOTH_ENABLED to false to disable BLE entirely.
const bool BLUETOOTH_ENABLED = true;
const char BLE_DEVICE_NAME[] = "Ojo del Sol Sensor";
const char BLE_MANUFACTURER[] = "Ojo del Sol";
// Stop pairing (or re-pairing after dropping connection) after 1 minute
const unsigned long BLE_PAIRING_TIMEOUT_MS = 60000;
const bool BLE_RESYNC_ENABLED = true;
const unsigned long BLE_RESYNC_INTERVAL_MS = 60000; // Clamp + refill interval

// -----------------------------------------------------------------------------
// USB HID
// -----------------------------------------------------------------------------
// Requires Arduino IDE board settings:
//   Tools > USB Mode: "USB-OTG (TinyUSB)"
//   Tools > USB CDC On Boot: "Enabled"
//   Tools > Upload Mode: "USB-OTG CDC (TinyUSB)" (for CDC-mode uploads)
// When true, normal runtime enumerates as USB XInput.
// USB CDC serial is entered from the DEBUG screen (hold 2s), then exits back
// to XInput-on-next-wake with another 2s hold.
const bool USB_HID_ENABLED = true;

// =============================================================================
// DEBUG
// =============================================================================
// Enables Serial debug logging (115200 baud) for startup diagnostics,
// device-button press events, and any enabled periodic debug streams below.
const bool DEBUG_SERIAL = true;
// Periodic Serial debug stream controls. These only apply when DEBUG_SERIAL = true.
const bool DEBUG_SERIAL_BATTERY = true;
const bool DEBUG_SERIAL_UV = true;

#endif
