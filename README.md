======================================================================
Ojo del Sol - ESP32-S3 UV Meter for Boktai Games
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
- 2x 100KΩ Resistors (for battery monitoring, optional)

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

BATTERY MONITORING (Optional):
The board does NOT have built-in battery voltage monitoring. To enable
battery percentage display, add a voltage divider circuit:

COMPONENTS:
- 2x 100KΩ resistors (1/4W, through-hole or SMD)

WIRING DIAGRAM:
```
  Battery (+) ───┬─── 5V pin (charging input)
                 │
               [R1] 100KΩ
                 │
                 ├─────── GP13 (ADC input)
                 │
               [R2] 100KΩ
                 │
  Battery (-) ───┴─── GND
```

HOW IT WORKS:
The two equal resistors form a 2:1 voltage divider:
- Full charge (4.2V) → GP13 reads 2.1V → displays 100%
- Empty (3.3V) → GP13 reads 1.65V → displays 0%

Without this circuit, battery % will always show 0%.

CALIBRATION:
If battery % is wrong when fully charged (4.2V), adjust VOLT_DIVIDER_MULT
in config.h:
- Default: 2.25 (calibrated for typical hardware variance)
- Formula: new = old × 4.2 ÷ (3.3 + displayed% × 0.009)
- Example: showing 48% → 2.0 × 4.2 ÷ (3.3 + 0.432) = 2.25
- Or simply: increase value if % too low, decrease if % too high

CHARGING DETECTION:
The firmware detects USB power by monitoring voltage. When USB is connected,
the 5V rail voltage exceeds what a LiPo can produce (>4.3V), triggering
charging mode:
- Display shows "CHG" instead of percentage
- Battery icon shows a filling animation
- When USB is unplugged, returns to normal battery percentage display

2. POWER BUTTON WIRING
----------------------------------------------------------------------
A single tactile push button controls power and game selection.

| Button Pin    | ESP32-S3 Mini Pin | Description               |
| :------------ | :---------------- | :------------------------ |
| Pin 1 (leg A) | GP2               | Signal (internal pull-up) |
| Pin 2 (leg B) | GND               | Ground                    |

BUTTON BEHAVIOR:
When device is ON:
- Tap (short press):  Cycle to next game (1 → 2 → 3 → 1...)
- Hold 3 seconds:     Power OFF (enters deep sleep ~10µA)

When waking from sleep:
- Press button:       Shows "Hold 3s to power on" for 10 seconds
- Hold 3 seconds:     Powers ON the device
- No activity 10s:    Screen turns off, returns to sleep

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

LTR390 UV Sensor (per datasheet DS86-2015-0004):
- Reference sensitivity: 2300 counts/UVI at 18x gain, 400ms
- UV Index formula: UVI = raw × (18/gain) × (400/int_time) / 2300
- At our settings (1x gain, 100ms): UVI = raw / 32
- Peak response: 300-350nm (matches solar UV-A/UV-B)
- NOT inverted: higher values = more UV
- Configured for: Gain 1x (max headroom), 18-bit/100ms resolution

LTR390 MODULE LED:
The LED on the Adafruit LTR390 breakout is a power indicator wired
directly to VIN. It is NOT controllable via software - it will always
be on when the sensor is powered, including during deep sleep.

RECOMMENDATION: Desolder or remove the LED to extend battery life.
The LED draws ~2-5mA continuously, which reduces sleep time from
months to just days. The LED serves no functional purpose - removing
it won't affect sensor operation.

IMPORTANT: Do not place the sensor behind glass or standard plastic!
Most glass blocks 90%+ of UV light. Use an open aperture, quartz glass,
or UV-transparent acrylic if an enclosure window is needed.

7. TROUBLESHOOTING
----------------------------------------------------------------------

UV SENSOR READS 0 OR VERY LOW:
1. Open Serial Monitor (115200 baud) to see raw sensor counts
   - At UVI 6, expect ~192 raw counts (6 × 32)
   - If raw counts are 0, sensor may not be in UV mode
2. Check sensor orientation - sensor window must face the sky
3. Ensure no glass/plastic blocks the sensor (blocks 90%+ of UV)
4. Low readings indoors are normal - indoor UV is near zero
5. UV flashlights vary greatly; test outside in direct sunlight

BATTERY ALWAYS SHOWS 0%:
You need a voltage divider circuit to monitor battery voltage.
See "BATTERY MONITORING" section in hardware connections.
Without this, the ADC pin reads near zero.

BATTERY % IS WRONG (e.g., 48% when fully charged):
Adjust VOLT_DIVIDER_MULT in config.h:
- If % is too LOW:  increase the multiplier
- If % is too HIGH: decrease the multiplier
- See BATTERY MONITORING section for exact formula
- Default 2.25 is calibrated for common hardware variance

DEVICE WON'T WAKE FROM SLEEP:
1. Press the button to show "Hold 3s to power on" prompt
2. Hold for 3 full seconds while the prompt is displayed
3. The prompt auto-hides after 10 seconds of inactivity
4. If screen is off, press button again to show the prompt

8. FUTURE PLANS
----------------------------------------------------------------------
- [ ] Bluetooth and/or HID output for emulators 
- [ ] GBA Link Port output for direct hardware integration
- [ ] Save selected game to NVS (persist across power cycles)

9. CREDITS & LINKS
----------------------------------------------------------------------
- GBATEK Solar Sensor Documentation:
  https://problemkaputt.de/gbatek.htm
- Prof9's Boktai ROM Hacks:
  (link to Prof9's patches when available)
- Adafruit LTR390:
  https://www.adafruit.com/product/4831

======================================================================
