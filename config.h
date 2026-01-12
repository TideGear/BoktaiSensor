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

// Raw UV Index display range (for the secondary readout)
const float UV_DISPLAY_MAX = 11.0;

// -----------------------------------------------------------------------------
// POWER BUTTON SETTINGS
// -----------------------------------------------------------------------------
// GP2 supports RTC wake-up from deep sleep.
const int BUTTON_PIN = 2;
const unsigned long DEBOUNCE_MS = 50;       // Button debounce time
const unsigned long LONG_PRESS_MS = 3000;   // Hold 3 seconds to power on/off

#endif
