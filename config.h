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
const float AUTO_UV_MAX = 9.0;   // UV Index for full bars (direct sun)

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
// BOKTAI 1 - 8 bars
const float BOKTAI_1_UV[8] = {
  0.5,   // Bar 1: Minimal UV (shade/overcast)
  1.2,   // Bar 2: Low UV
  2.0,   // Bar 3: Moderate UV (light clouds)
  3.0,   // Bar 4: Moderate UV
  4.0,   // Bar 5: High UV (partly sunny)
  5.5,   // Bar 6: High UV (sunny)
  7.0,   // Bar 7: Very high UV
  9.0    // Bar 8: Extreme UV (direct midday sun)
};

// BOKTAI 2 - 10 bars
const float BOKTAI_2_UV[10] = {
  0.4,   // Bar 1
  0.8,   // Bar 2
  1.4,   // Bar 3
  2.0,   // Bar 4
  2.8,   // Bar 5
  3.6,   // Bar 6
  4.5,   // Bar 7
  5.5,   // Bar 8
  7.0,   // Bar 9
  9.0    // Bar 10
};

// BOKTAI 3 - 10 bars
const float BOKTAI_3_UV[10] = {
  0.4,   // Bar 1
  0.8,   // Bar 2
  1.4,   // Bar 3
  2.0,   // Bar 4
  2.8,   // Bar 5
  3.6,   // Bar 6
  4.5,   // Bar 7
  5.5,   // Bar 8
  7.0,   // Bar 9
  9.0    // Bar 10
};

// Raw UV Index display range (for the secondary readout)
const float UV_DISPLAY_MAX = 11.0;

// Battery Monitoring
// GP1 is an ADC pin used to sense battery voltage.
const int BAT_PIN = 1; 
const float VOLT_MIN = 3.3;  // 0% Battery
const float VOLT_MAX = 4.2;  // 100% Battery

// Power Button Settings
// GP2 supports RTC wake-up from deep sleep.
const int BUTTON_PIN = 2;
const unsigned long DEBOUNCE_MS = 50;       // Button debounce time
const unsigned long LONG_PRESS_MS = 3000;   // Hold 3 seconds to power on/off

#endif