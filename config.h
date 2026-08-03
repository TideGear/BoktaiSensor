// config.h - Thresholds and Pin Settings
#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// BOARD
// =============================================================================
// This branch targets the LilyGO Mini E-Paper S3 exclusively.
//
// LILYGO Mini E-Paper S3 (ESP32-S3-FN4R2, 4MB flash + 2MB PSRAM) with a
// built-in 1.02" GDEW0102T4 (UC8175) 80x128 e-paper panel, a 3-way tactile
// switch (left / press / right), a built-in battery voltage divider, and a
// microSD slot (unused here).
//
// The LTR390 connects to the IO7/IO8 header pads (I2C). The GBA link uses the
// IO16/IO17/IO18 header pads with the adjacent GND pad.
//
// Requires the "GxEPD2" library (ZinggJM) from the Arduino Library Manager;
// no library config files need editing.
//
// UNTESTED on real hardware. See the pin notes below for the two vendor
// documentation conflicts to check first if the panel or battery misbehaves.
#define BOARD_LILYGO_MINI_EPAPER_S3

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
// UV ENCLOSURE COMPENSATION — NOT PRESENT ON THIS BRANCH
// -----------------------------------------------------------------------------
// The Mini E-Paper S3 build runs the LTR390 exposed (no case window over the
// sensor), so there is no UV transmission loss to model and the compensation
// pass has been removed rather than left as a no-op. Threshold values below are
// therefore plain open-air UVI readings.
//
// If you later put the sensor behind a window, the compensation code is on the
// main branch (UV_ENCLOSURE_COMP_ENABLED / UV_ENCLOSURE_TRANSMITTANCE /
// UV_ENCLOSURE_UVI_OFFSET / UV_THRESHOLDS_CALIBRATED_OPEN_AIR).

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

// =============================================================================
// POWER MANAGEMENT
// =============================================================================

// -----------------------------------------------------------------------------
// BATTERY MONITORING
// -----------------------------------------------------------------------------
// The Mini E-Paper S3 has a built-in battery voltage divider (2:1) on GPIO2.
// No external wiring is needed.
//
// PIN CONFLICT: LilyGO's vendor board header (lib/GxEPD/src/boards.h) defines
// ADC_PIN as GPIO2, while the product-page pinout image labels the battery ADC
// as IO43. GPIO43 is U0TXD and has no ADC channel on the ESP32-S3 (ADC1 is
// GPIO1-10, ADC2 is GPIO11-20), so it cannot be read with analogRead() and the
// image is taken to be wrong. If battery readings are implausible, this is the
// first thing to check against your board revision.
const bool BATTERY_SENSE_ENABLED = true;
const int BAT_PIN = 2;       // GPIO2 - built-in battery voltage divider
const float VOLT_MIN = 3.3;  // 0% Battery
const float VOLT_MAX = 4.2;  // 100% Battery (must be > VOLT_MIN)
const unsigned long BATTERY_SAMPLE_MS = 1000;  // Battery update interval

// Voltage divider calibration multiplier
// Theoretical: 2.0 for equal resistors (2:1 ratio)
// Adjust if battery % is wrong at full charge:
//   - If % too LOW:  increase this value
//   - If % too HIGH: decrease this value
const float VOLT_DIVIDER_MULT = 2.00;  // Built-in divider is 2:1

// -----------------------------------------------------------------------------
// LOW-VOLTAGE CUTOFF (requires battery sense divider)
// -----------------------------------------------------------------------------
// Keep disabled by default; only enable after battery sensing is verified.
const bool BATTERY_CUTOFF_ENABLED = true;
const float VOLT_CUTOFF = VOLT_MIN;  // Cutoff threshold under load
const unsigned long BATTERY_CUTOFF_HOLD_MS = 5000;

// =============================================================================
// USER INTERFACE
// =============================================================================

// -----------------------------------------------------------------------------
// BUTTONS
// -----------------------------------------------------------------------------
// The board's 3-way tactile switch on the top edge: press = GPIO0,
// left = GPIO3, right = GPIO4. All three are RTC-capable pads, but only the
// power button can wake the ESP32-S3 from deep sleep, so the center press
// (GPIO0) takes that role: hold 2s to sleep/wake.
//
// GPIO0 is also the boot/download button. That is why the vendor's flash
// instructions are "hold the button while plugging in USB" — it is the same
// switch, and holding it at reset enters download mode instead of running.
//
// Tap directions: center or right advances to the next screen, left goes back.
// Adjust the *_TAP_STEP values to change this.
const int BUTTON_PIN = 0;     // Center press (GPIO0) - power; tap = next screen
const int BUTTON_TAP_STEP = 1;

const bool BUTTON2_ENABLED = true;
const int BUTTON2_PIN = 4;    // Right (GPIO4) - tap = next screen
const int BUTTON2_TAP_STEP = 1;

const bool BUTTON3_ENABLED = true;
const int BUTTON3_PIN = 3;    // Left (GPIO3) - tap = previous screen
const int BUTTON3_TAP_STEP = -1;

const unsigned long DEBOUNCE_MS = 50;       // Button debounce time
const unsigned long LONG_PRESS_MS = 2000;   // Hold 2 seconds to power on/off

// -----------------------------------------------------------------------------
// E-PAPER DISPLAY
// -----------------------------------------------------------------------------
// Fixed board wiring for the built-in GDEW0102T4 panel.
const int EPD_MOSI_PIN = 15;
const int EPD_MISO_PIN = -1;   // Panel is write-only
const int EPD_SCLK_PIN = 14;
const int EPD_CS_PIN = 13;
const int EPD_DC_PIN = 12;
const int EPD_RST_PIN = 11;
const int EPD_BUSY_PIN = 10;

// Panel power rail enable.
//
// PIN CONFLICT: LilyGO's vendor board header defines EPD_POWER_ENABLE as
// GPIO42 (and GPIO21 as an unrelated MOTOR pin, broken out on the header),
// while the product-page pinout image labels the display POWER pin as 21. The
// vendor header is what LilyGO's own working examples and shipped firmware
// use, so GPIO42 is the default. If the panel stays blank with no other
// symptom, try 21 here first.
//
// Note: the firmware latches this pin off through deep sleep with
// gpio_hold_en() + gpio_deep_sleep_hold_en(), which is the correct mechanism
// for a digital-only pad like GPIO42. If you switch it to an RTC pad (GPIO0-21,
// including 21) the latch may not survive deep sleep and the rail could stay
// powered. That is harmless here — the panel is hibernated first, so it draws
// about 1uA either way.
const int EPD_POWER_PIN = 42;
const bool EPD_POWER_ACTIVE_HIGH = true;

// Panel rotation. The 128-pixel-wide UI only fits across the panel's long
// axis, so only the landscape rotations are usable: 1 or 3 (they are 180
// degrees apart). Pick whichever matches your case orientation.
const uint8_t EPAPER_DISPLAY_ROTATION = 1;

// Minimum time between ROUTINE panel refreshes. E-paper refreshes take hundreds
// of milliseconds and the panel wears with each one, so the UI cannot be redrawn
// as freely as an OLED. Frames that arrive sooner are held and the newest one is
// pushed when the interval elapses; frames identical to what is already on
// screen are skipped entirely.
//
// This governs incidental changes only — the UVI decimals ticking over, the
// battery percentage moving. The sun gauge does NOT wait on it; see
// EPAPER_PRIORITY_MIN_REFRESH_MS below. Since the gauge is independent, raising
// this is a cheap way to cut panel wear: the only cost is a staler UVI readout
// on the game screens (the XInput screen is there when you need precision).
const unsigned long EPAPER_MIN_REFRESH_MS = 1500;

// Minimum time between PRIORITY refreshes: a changed bar count, or a screen
// change the user just pressed a button for. These bypass the interval above so
// the gauge tracks the sensor as closely as the panel physically allows.
//
// 0 = no floor, the fastest the hardware can go. That is not unbounded, and the
// hardware is the real limit here: GxEPD2 budgets up to 1200ms for a partial
// refresh of this panel, and each refresh blocks until BUSY clears. So the
// gauge steps at roughly one bar per second no matter what this is set to. (The
// LTR390's 500ms measurement rate is the other, looser cap.)
//
// Because the panel floor is close to EPAPER_MIN_REFRESH_MS above, this setting
// buys less than it looks like it should — it removes the interval wait, not the
// refresh itself.
//
// Raise this only if the extra refreshes bother you. Note that if the bar count
// oscillates between two levels in borderline light, BAR_HYSTERESIS_ENABLED
// above is the better fix — it stops the oscillation instead of hiding it.
const unsigned long EPAPER_PRIORITY_MIN_REFRESH_MS = 0;

// Repeated partial refreshes leave faint ghosting behind. Every Nth refresh is
// promoted to a full refresh (slower, flashes black/white) to clear it.
// 0 disables full refreshes entirely.
const uint16_t EPAPER_FULL_REFRESH_EVERY = 30;

// -----------------------------------------------------------------------------
// I2C (LTR390 UV SENSOR)
// -----------------------------------------------------------------------------
// The board has no Qwiic/STEMMA connector, so the LTR390 breakout is wired to
// the IO7/IO8 header pads (plus 3v3 and GND). Both lines need pull-ups; the
// Adafruit LTR390 breakout has them on board.
const int I2C_SDA_PIN = 7;                // IO7 pad
const int I2C_SCL_PIN = 8;                // IO8 pad

// -----------------------------------------------------------------------------
// NO-SENSOR BAR DEMO
// -----------------------------------------------------------------------------
// When no LTR390 answered at boot, drive the gauge through a repeating demo as
// fast as the panel can redraw instead of leaving it pinned at 0:
//
//   1. Walk  - one bar at a time, 0 -> max -> 0
//   2. Flash - every bar on and off together, NO_SENSOR_BAR_SWEEP_FLASHES times
//   ...then loop.
//
// This doubles as a panel benchmark — the demo advances one step per completed
// refresh, so the visible rate IS the panel's real refresh rate — and as an
// end-to-end test of the output chain, since the demo value is also sent over
// Bluetooth/USB HID and the GBA link. The walk phase exercises single-bar
// transitions; the flash phase exercises full-scale jumps.
//
// Set false to leave the gauge at 0 instead. Do that if you are testing HID
// pairing or emulator mapping and a swinging meter would get in the way.
//
// Note this refreshes the panel continuously for as long as the device is on
// with no sensor attached, which is real panel wear. It is a bring-up and demo
// aid, not something to leave running for hours.
const bool NO_SENSOR_BAR_SWEEP = true;

// All-bars-on/off blinks between each walk. 0 skips the flash phase, leaving
// just the up-and-down walk. Each flash costs two panel refreshes, so a full
// sequence is 2 * (bars + flashes) refreshes.
const uint8_t NO_SENSOR_BAR_SWEEP_FLASHES = 3;

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
//
// Uses the IO16/IO17/IO18 header pads (GND is available on the same headers).
// Other exposed pads (IO5, IO6, IO9, IO17, IO18, IO21) also work if these are
// inconvenient for your build; avoid IO7/IO8 (I2C) and the SD card pads
// (IO38-IO41).
const int GBA_PIN_SC = 16;  // IO16 pad - frame phase (SC)
const int GBA_PIN_SD = 17;  // IO17 pad - payload bit 1 (SD)
const int GBA_PIN_SO = 18;  // IO18 pad - payload bit 0 (SO)
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
const uint8_t HID_CONTROL_MODE = 1;

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
const bool DEBUG_SERIAL_BATTERY = false;
const bool DEBUG_SERIAL_UV = true;
// Logs how long each panel refresh actually blocked on the panel's BUSY line,
// and whether it was a partial or a full refresh. This is the hard floor on how
// fast the gauge can change, so measure it rather than trusting GxEPD2's
// conservative budgets (1200ms partial / 3600ms full for this panel).
// Pair with NO_SENSOR_BAR_SWEEP for a continuous stream of samples.
const bool DEBUG_SERIAL_EPAPER = true;

#endif
