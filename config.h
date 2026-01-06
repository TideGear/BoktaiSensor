// config.h - Thresholds and Pin Settings
#ifndef CONFIG_H
#define CONFIG_H

// UV Meter Calibration
// UV Index typically ranges from 0 to 11.
const float UV_MIN = 0.5;   // Below this, meters are empty
const float UV_MAX = 10.0;  // At or above this, meters are full

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