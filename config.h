// config.h - Thresholds and Pin Settings
#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// BOARD SELECTION
// =============================================================================
// Uncomment exactly ONE board.
//
// BOARD_SEEED_XIAO_ESP32S3: Seeed XIAO ESP32S3 with external SSD1306 OLED,
//   external button, and optional GBA link port / battery divider (original build).
//
// BOARD_LILYGO_T_QT_PRO: LilyGO T-QT Pro (ESP32-S3) with built-in 0.85"
//   GC9107 128x128 TFT, two built-in buttons, and built-in battery sensing.
//   The LTR390 connects to the Qwiic connector. Requires the "GFX Library
//   for Arduino" (Arduino_GFX by moononournation); no library config needed.
//   The GBA link uses the IO16/IO17/IO18 solder pads (GND pad adjacent).
//   Pin assignments follow LilyGO's official pinmap/schematic. Tested on
//   real hardware; the GBA link pads are the least-exercised part.
//#define BOARD_SEEED_XIAO_ESP32S3
#define BOARD_LILYGO_T_QT_PRO

#if defined(BOARD_SEEED_XIAO_ESP32S3) && defined(BOARD_LILYGO_T_QT_PRO)
#error "Select only one board in config.h"
#elif !defined(BOARD_SEEED_XIAO_ESP32S3) && !defined(BOARD_LILYGO_T_QT_PRO)
#error "Select a board in config.h"
#endif

// =============================================================================
// SENSOR SELECTION
// =============================================================================
// Uncomment exactly ONE sensor.
//
// SENSOR_LTR390: Adafruit LTR390 UV sensor (default). Bars come from the UV
//   index and the calibration settings below.
//
// SENSOR_GBA_CARTRIDGE: UNTESTED. A real Boktai cartridge's own solar sensor,
//   read over the GBA cartridge bus (T-QT Pro only). Bars are computed from
//   the cartridge's reading with the games' level formula and bar tables, so
//   the UV calibration, enclosure and filtering settings below are not used.
//   It still needs a dark-baseline value (CART_CALIBRATION in CARTRIDGE
//   SENSOR). It reads with the Boktai 2/3 sequence for every game; Boktai 1's
//   differs slightly and may read one count differently. The reading method is
//   the Ojo del Sol Calibrator's, which has not been run on an ESP32 yet. See
//   the README for the wiring.
#define SENSOR_LTR390
//#define SENSOR_GBA_CARTRIDGE

#if defined(SENSOR_LTR390) && defined(SENSOR_GBA_CARTRIDGE)
#error "Select only one sensor in config.h"
#elif !defined(SENSOR_LTR390) && !defined(SENSOR_GBA_CARTRIDGE)
#error "Select a sensor in config.h"
#elif defined(SENSOR_GBA_CARTRIDGE) && !defined(BOARD_LILYGO_T_QT_PRO)
#error "SENSOR_GBA_CARTRIDGE is only supported on BOARD_LILYGO_T_QT_PRO"
#endif

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

const float AUTO_UV_MIN = 1.000;   // UV Index for 1 bar (shade/overcast)
const float AUTO_UV_SATURATION = 13.100;   // UV Index where output is fully saturated/clamped

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
const float UVI_SMOOTHING_ALPHA = 0.5;  // 0.0-1.0. Controls how much weight new readings get vs. the running average. Higher = faster response.
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
const float BOKTAI_1_UV_MIN = 1.000;
const float BOKTAI_1_UV_SATURATION = 13.100;

const float BOKTAI_2_UV_MIN = 1.000;
const float BOKTAI_2_UV_SATURATION = 13.100;

// Intentionally separate from BOKTAI_2 for possible future divergence.
const float BOKTAI_3_UV_MIN = 1.000;
const float BOKTAI_3_UV_SATURATION = 13.100;

// -----------------------------------------------------------------------------
// CARTRIDGE SENSOR (only used with SENSOR_GBA_CARTRIDGE) - UNTESTED
// -----------------------------------------------------------------------------
#if defined(SENSOR_GBA_CARTRIDGE)
// Bus pins on the T-QT Pro's solder pads and Qwiic connector (the LTR390 is
// not used, so the Qwiic pins are free; its 3V3/GND pins can power the
// cartridge). The active-low control lines are kept off GPIO43, the boot
// ROM's serial output, which toggles during boot. IO16/IO17/IO18 stay free for the GBA link output.
// AD4, AD7-AD15, A16-A23 and /CS2 are tied off externally; see the README.
const uint8_t CART_AD_PINS[6] = { 36, 37, 38, 39, 43, 44 };  // AD0, AD1, AD2, AD3, AD5, AD6
const uint8_t CART_WR_PIN = 33;
const uint8_t CART_RD_PIN = 34;
const uint8_t CART_CS_PIN = 35;

// Optional cartridge power switch: a P-channel MOSFET between 3V3 and the
// cartridge's VCC, gate on this pin with a ~100k pull-up to 3V3 (see the
// README). The firmware drives it LOW to power the cartridge only while the
// device is on, and the pull-up keeps it off during deep sleep. Harmless if
// the switch isn't fitted (the cartridge is then always powered); set -1 to
// leave the pin alone.
const int CART_POWER_PIN = 48;
const unsigned long CART_POWER_ON_MS = 20;  // Settle time after switching on

// The game turns the cartridge's raw reading into a 0-140 level with
//   level = CART_CALIBRATION - raw   (limited to 0-140)
// and maps the level to bars with its own table. Raw is about 230 in the dark
// and falls toward 0 in bright light. The game stores a reading taken during
// its sensor setup, minus 2, as its calibration (seen in the ROM). Whether
// that reading is meant to be taken in the dark is not confirmed; covering
// the sensor is the working assumption. So: cover the sensor, read "Raw" on
// the XInput/CDC screen, and set this to that value minus 2. The default of
// 230 is only a starting guess.
const int CART_CALIBRATION = 230;

// Boktai 1 (E) uses a different bar table from Boktai 1 (J/U). Set true to
// match the European release on the BOKTAI 1 screen.
const bool CART_BOKTAI1_EUROPE_TABLE = false;

// Time between the starts of two readings. A reading takes up to ~0.4 s
// (longest in the dark).
const unsigned long CART_READ_INTERVAL_MS = 500;

// Time per sensor tick; two ticks make one clock pulse. The game clocks the
// sensor from a timer that fires about every 750 microseconds. Raise this if
// the readings look unstable.
const uint32_t CART_TICK_US = 750;

// Ticks run from a hardware timer interrupt on the loop core, away from the
// Bluetooth stack, but flash writes (e.g. saving a Bluetooth pairing) can
// still delay one. A reading with any tick later than this is discarded and
// the previous level kept. 0 keeps every reading.
const uint32_t CART_TICK_MAX_LATE_US = 250;
#endif

// =============================================================================
// POWER MANAGEMENT
// =============================================================================

// -----------------------------------------------------------------------------
// BATTERY MONITORING
// -----------------------------------------------------------------------------
#if defined(BOARD_LILYGO_T_QT_PRO)
// The T-QT Pro has a built-in battery voltage divider (2:1) on GPIO4.
// No external wiring is needed.
const bool BATTERY_SENSE_ENABLED = true;
const int BAT_PIN = 4;       // GPIO4 - built-in battery voltage divider
#else
// NOTE: The XIAO ESP32S3 does NOT have built-in battery voltage monitoring!
// You must add a voltage divider (2x 100K resistors) from BAT+ to GND with
// the midpoint connected to BAT_PIN. Without this, battery % is unavailable.
// Safe default is disabled until the divider is wired.
const bool BATTERY_SENSE_ENABLED = false;
const int BAT_PIN = 2;       // D1/A1 (GPIO2) - connect to voltage divider midpoint
#endif
const float VOLT_MIN = 3.3;  // 0% Battery
const float VOLT_MAX = 4.2;  // 100% Battery (must be > VOLT_MIN)
const unsigned long BATTERY_SAMPLE_MS = 1000;  // Battery update interval

// Voltage divider calibration multiplier
// Theoretical: 2.0 for equal resistors (2:1 ratio)
// Adjust if battery % is wrong at full charge:
//   - If % too LOW:  increase this value
//   - If % too HIGH: decrease this value
#if defined(BOARD_LILYGO_T_QT_PRO)
const float VOLT_DIVIDER_MULT = 2.00;  // Built-in divider is 2:1
#else
// Default 2.20 compensates for typical ADC/resistor variance on the XIAO build
const float VOLT_DIVIDER_MULT = 2.20;
#endif

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
#if defined(BOARD_LILYGO_T_QT_PRO)
// T-QT Pro built-in buttons. Only GPIO0 (left button) can wake the ESP32-S3
// from deep sleep, so it must be the power button (hold 2s to sleep/wake).
// Tap directions: right button advances to the next screen, left button
// goes back. Adjust the *_TAP_STEP values to change this.
const int BUTTON_PIN = 0;     // BTN_L (GPIO0) - power; tap = previous screen
const bool BUTTON2_ENABLED = true;
const int BUTTON2_PIN = 47;   // BTN_R (GPIO47) - tap = next screen
// Screens cycled per tap (+1 = forward, -1 = backward)
const int BUTTON_TAP_STEP = -1;
const int BUTTON2_TAP_STEP = 1;
#else
// D0 (GPIO1) supports RTC wake-up from deep sleep on XIAO ESP32S3.
const int BUTTON_PIN = 1;    // D0 (GPIO1)
const int BUTTON_TAP_STEP = 1;  // Single button always advances forward
// No second button on the XIAO build.
const bool BUTTON2_ENABLED = false;
const int BUTTON2_PIN = -1;
const int BUTTON2_TAP_STEP = 1;
#endif
const unsigned long DEBOUNCE_MS = 50;       // Button debounce time
const unsigned long LONG_PRESS_MS = 2000;   // Hold 2 seconds to power on/off

// -----------------------------------------------------------------------------
// DISPLAY / I2C
// -----------------------------------------------------------------------------
#if defined(BOARD_LILYGO_T_QT_PRO)
// LTR390 I2C on the Qwiic connector (Adafruit's STEMMA QT LTR390 plugs
// straight in, no soldering). Unused with SENSOR_GBA_CARTRIDGE. NOTE: the board silkscreen/pinout image swaps
// these labels; the schematic -- and working I2C -- is SDA = GPIO43,
// SCL = GPIO44.
//
// Solder-pad alternative: set SDA = 33 and SCL = 34 to use the IO33/IO34
// pads instead. Pull-ups are then required on both lines; the Adafruit
// LTR390 breakout has them on board.
const int I2C_SDA_PIN = 43;               // Qwiic pin 3 (U0TXD/GPIO43)
const int I2C_SCL_PIN = 44;               // Qwiic pin 4 (U0RXD/GPIO44)
// Built-in GC9107 TFT. SPI pins are set in TQTDisplay.h (they are fixed
// board wiring); only the backlight is managed here.
const int TFT_BACKLIGHT_PIN = 10;         // Active LOW
// 0-3. Rotates the UI on the square screen if your case orientation differs.
const uint8_t TQT_DISPLAY_ROTATION = 0;
#else
const int I2C_SDA_PIN = 5;                // D4 (GPIO5)
const int I2C_SCL_PIN = 6;                // D5 (GPIO6)
const uint8_t DISPLAY_I2C_ADDR = 0x3C;   // Some SSD1306 modules use 0x3D
#endif

// -----------------------------------------------------------------------------
// DISPLAY SCREENSAVER
// -----------------------------------------------------------------------------
// After SCREENSAVER_TIME minutes of no button activity, show a bouncing
// "Ojo del Sol" screensaver. Disabled by default on the T-QT Pro: its LCD
// isn't prone to OLED-style burn-in, and the screensaver leaves the backlight
// on, so it saves no power there either.
#if defined(BOARD_LILYGO_T_QT_PRO)
const bool SCREENSAVER_ACTIVE = false;
#else
const bool SCREENSAVER_ACTIVE = true;
#endif
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
#if defined(BOARD_LILYGO_T_QT_PRO)
// Uses the solder pads next to the GND pad on the board edge. Other exposed
// pads (IO33-IO39, IO48) also work if these are inconvenient for your build,
// except with SENSOR_GBA_CARTRIDGE, which uses them all.
const int GBA_PIN_SC = 16;  // IO16 pad - frame phase (SC)
const int GBA_PIN_SD = 17;  // IO17 pad - payload bit 1 (SD)
const int GBA_PIN_SO = 18;  // IO18 pad - payload bit 0 (SO)
#else
const int GBA_PIN_SC = 44;  // D7 (GPIO44) - frame phase (SC)
const int GBA_PIN_SD = 7;   // D8 (GPIO7)  - payload bit 1 (SD)
const int GBA_PIN_SO = 8;   // D9 (GPIO8)  - payload bit 0 (SO)
#endif
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
const bool DEBUG_SERIAL = false;
// Periodic Serial debug stream controls. These only apply when DEBUG_SERIAL = true.
const bool DEBUG_SERIAL_BATTERY = true;
const bool DEBUG_SERIAL_UV = true;  // UV readings, or cartridge readings with SENSOR_GBA_CARTRIDGE

#endif
