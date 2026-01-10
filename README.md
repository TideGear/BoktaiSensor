======================================================================
BOKTAI SOLAR SENSOR - ESP32-S3 UV Meter for Boktai Games
======================================================================

A substitute solar sensor for playing the Boktai series (Boktai 1, 2, 3)
on flash carts or emulators. Restores "Kojima's Intent" by using real
sunlight instead of artificial light hacks.

SUPPORTED GAMES
---------------
- Boktai 1: The Sun Is in Your Hand (8-bar gauge)
- Boktai 2: Solar Boy Django (10-bar gauge)
- Boktai 3: Sabata's Counterattack (10-bar gauge)

HARDWARE
--------
- Waveshare ESP32-S3 Mini
- Adafruit LTR390 UV Sensor (I2C)
- SSD1306 128x64 OLED Display (I2C)
- Tactile Push Button (power/game select)
- 3.7V LiPo Battery (450mAh recommended)

1. HARDWARE CONNECTIONS (I2C)
----------------------------------------------------------------------
Both the LTR390 sensor and the OLED display share the I2C bus.

| Component Pin | ESP32-S3 Mini Pin | Description            |
| :------------ | :---------------- | :--------------------- |
| OLED/LTR VIN  | 3V3 (OUT)         | 3.3V Power             |
| OLED/LTR GND  | GND               | Common Ground          |
| OLED/LTR SDA  | GP8               | I2C Data               |
| OLED/LTR SCL  | GP9               | I2C Clock              |
| Battery Red   | 5V                | Positive (+) Term      |
| Battery Black | GND               | Negative (-) Term      |

IMPORTANT: The 5V pin on this board is the battery input/charging pin. 
Connecting your 3.7V LiPo here allows the board to charge via USB-C.

2. POWER BUTTON WIRING
----------------------------------------------------------------------
A single tactile push button controls power and game selection.

| Button Pin    | ESP32-S3 Mini Pin | Description               |
| :------------ | :---------------- | :------------------------ |
| Pin 1 (leg A) | GP2               | Signal (internal pull-up) |
| Pin 2 (leg B) | GND               | Ground                    |

BUTTON BEHAVIOR:
- Tap (short press):  Cycle to next game (1 → 2 → 3 → 1...)
- Hold 3 seconds:     Power ON from sleep
- Hold 3 seconds:     Power OFF (enters deep sleep ~10µA)

3. LIBRARIES REQUIRED
----------------------------------------------------------------------
Install via Arduino Library Manager:
- Adafruit LTR390 Library
- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit BusIO

4. USAGE
----------------------------------------------------------------------
1. Power on the device (hold button 3 seconds)
2. Tap button to select your game (BOKTAI 1, 2, or 3)
3. Point the LTR390 sensor toward the sky/sun
4. Read the bar count from the display
5. Enter the matching sun level in-game using Prof9's ROM hacks
   (or your flash cart's input method)
6. Power off when done (hold button 3 seconds)

Board Settings (Arduino IDE):
- Board: "ESP32S3 Dev Module" or "Waveshare ESP32-S3-Zero"
- USB CDC On Boot: Enabled (for serial debugging)

5. CALIBRATION
----------------------------------------------------------------------

AUTOMATIC MODE (Default)
------------------------
By default, AUTO_MODE is enabled in config.h. This uses two simple
values to calculate bar thresholds for all games:

  const bool AUTO_MODE = true;
  const float AUTO_UV_MIN = 0.5;   // UV for 1 bar
  const float AUTO_UV_MAX = 6.0;   // UV for full bars

The code automatically distributes bars evenly across this range:
- Boktai 1:   8 bars spread from 0.5 to 6.0
- Boktai 2/3: 10 bars spread from 0.5 to 6.0

Note: UV Index 6.0 is typical for a sunny day in temperate climates
(Japan, US, Europe). This default was chosen so a full gauge is
achievable under normal sunny conditions without requiring extreme UV.

To calibrate in auto mode, simply adjust UV_MIN and UV_MAX based on
what your original cartridge shows at low and high sunlight levels.

MANUAL MODE (Advanced)
----------------------
Set AUTO_MODE = false to use per-game threshold arrays:
- BOKTAI_1_UV[8]  - 8 custom thresholds for Boktai 1
- BOKTAI_2_UV[10] - 10 custom thresholds for Boktai 2
- BOKTAI_3_UV[10] - 10 custom thresholds for Boktai 3

This allows fine-tuning each bar level independently and per-game
if testing reveals differences between the games.

To calibrate:
1. Take device and original Boktai cart outside
2. Note UV Index reading and compare to in-game gauge
3. In auto mode: adjust AUTO_UV_MIN and AUTO_UV_MAX
   In manual mode: adjust individual threshold values
4. Repeat under different conditions (clouds, shade, direct sun)

6. TECHNICAL NOTES
----------------------------------------------------------------------
Original Boktai Cartridge Sensor (from GBATEK):
- Uses photodiode with 8-bit ADC (digital-ramp converter)
- Values are INVERTED: 0xE8 = darkness, 0x50 = max gauge, 0x00 = extreme
- Accessed via GPIO at 0x80000C4-0x80000C8

LTR390 UV Sensor:
- Raw UV counts converted to UV Index via formula
- UV Index formula: UVI = raw / (sensitivity × gain × int_time_factor)
- Peak response: 300-350nm (matches solar UV-A/UV-B)
- NOT inverted: higher values = more UV
- Configured for: Gain 1x (max headroom for direct sun), 13-bit (fastest)

IMPORTANT: Do not place the sensor behind glass or standard plastic!
Most glass blocks 90%+ of UV light. Use an open aperture, quartz glass,
or UV-transparent acrylic if an enclosure window is needed.

7. FUTURE PLANS
----------------------------------------------------------------------
- [ ] Bluetooth HID output for mGBA/smartphone emulators
- [ ] USB HID output for PC emulators  
- [ ] GBA Link Port output for direct hardware integration
- [ ] Runtime calibration mode (button-triggered dark/bright reference)
- [ ] Save selected game to NVS (persist across power cycles)

8. CREDITS & LINKS
----------------------------------------------------------------------
- GBATEK Solar Sensor Documentation:
  https://problemkaputt.de/gbatek.htm
- Prof9's Boktai ROM Hacks:
  (link to Prof9's patches when available)
- Adafruit LTR390:
  https://www.adafruit.com/product/4831

======================================================================
