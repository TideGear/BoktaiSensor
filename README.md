**This is all very new, experimental, and subject to change. I'm looking for serious contributors to...**

**1. Help calibrate the device**\
**2. Add support via Chord Control Mode to their emulators**\
**3. Test and fix the code if needed**\
**4. Create a cool 3D-printable case**\
**5. You tell me!**

**...so please contact me if your are interested at TideGear @at@ gmail .dot. com !**

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
- Seeed XIAO ESP32S3 (built-in LiPo charging)
- Adafruit LTR390 UV Sensor (I2C)
- SSD1306 128x64 OLED Display (I2C)
- Tactile Push Button (power/game select)
- 3.7V LiPo Battery (450mAh recommended)
- 2x 100K ohm Resistors (for battery monitoring)

1. HARDWARE CONNECTIONS (I2C)
----------------------------------------------------------------------
Both the LTR390 sensor and the OLED display share the I2C bus.

| Component Pin | XIAO ESP32S3 Pin  | Description                             |
| :------------ | :---------------- | :-------------------------------------- |
| OLED VIN      | 3V3               | 3.3V Power                              |
| OLED GND      | GND               | Common Ground                           |
| LTR390 VIN    | D3 (GPIO4)        | Sensor power (GPIO-controlled, default) |
| LTR390 GND    | GND               | Common Ground                           |
| OLED/LTR SDA  | D4 (GPIO5)        | I2C Data                                |
| OLED/LTR SCL  | D5 (GPIO6)        | I2C Clock                               |
| Battery Red   | BAT+ (back pad)   | Positive (+) Term                       |
| Battery Black | BAT- (back pad)   | Negative (-) Term                       |

BATTERY: Solder LiPo wires directly to BAT+ and BAT- pads on the back
of the XIAO. Built-in charging circuit handles USB-C charging. The BAT-
pad is closest to the USB-C port and BAT+ is farthest from the USB-C port.

SENSOR POWER DEFAULT:
SENSOR_POWER_ENABLED is true by default. Wire LTR390 VIN to D3 (GPIO4).
If you prefer always-on sensor power, set SENSOR_POWER_ENABLED = false
and connect LTR390 VIN to 3V3 instead.

BATTERY MONITORING:
The XIAO does NOT have built-in battery voltage monitoring. To enable
battery percentage display, add a voltage divider circuit:

COMPONENTS:
- 2x 100K ohm resistors (1/4W, through-hole or SMD)

WIRING DIAGRAM:
```
  BAT+ pad ----[R1 100K]----+---- D1 (GPIO2/A1, ADC input)
                             |
                           [R2 100K]
                             |
  BAT- pad ------------------+---- GND
```

HOW IT WORKS:
The two equal resistors form a 2:1 voltage divider:
- Full charge (4.2V) -> D1 reads 2.1V -> displays 100%
- Empty (3.3V) -> D1 reads 1.65V -> displays 0%

Without this circuit, battery % is unavailable. Set BATTERY_SENSE_ENABLED
in config.h to false to avoid floating ADC readings and hide the battery
indicator.

NOTE: The XIAO does not expose battery voltage on a dedicated GPIO.
Battery monitoring only works if you wire BAT+ through a divider into
an ADC pin (D1 in the diagram). A floating ADC pin will produce
unreliable readings.

LOW-VOLTAGE CUTOFF:
If BATTERY_CUTOFF_ENABLED is true, the firmware enters deep sleep when
battery voltage stays below VOLT_CUTOFF for BATTERY_CUTOFF_HOLD_MS.
This requires the voltage divider above. Set BATTERY_CUTOFF_ENABLED
to false if you don't use the divider or want to disable cutoff.
A brief "LOW BATTERY" message is shown before sleep.

CALIBRATION:
If battery % is wrong when fully charged (4.2V), adjust VOLT_DIVIDER_MULT
in config.h:
- Default: 2.25 (calibrated for typical hardware variance)
- Formula: new = old x 4.2 / (3.3 + displayed% x 0.009)
- Example: showing 48% -> 2.0 x 4.2 / (3.3 + 0.432) = 2.25
- Or simply: increase value if % too low, decrease if % too high

2. POWER BUTTON WIRING
----------------------------------------------------------------------
A single tactile push button controls power and game selection.

| Button Pin    | XIAO ESP32S3 Pin  | Description               |
| :------------ | :---------------- | :------------------------ |
| Pin 1 (leg A) | D0 (GPIO1)        | Signal (internal pull-up) |
| Pin 2 (leg B) | GND               | Ground                    |

BUTTON BEHAVIOR:
When device is ON:
- Tap (short press):  Cycle to next game (1 -> 2 -> 3 -> 1...)
- Hold 3 seconds:     Power OFF (enters deep sleep ~10uA)
- If screensaver is active: press to wake the screen; next tap changes game

When waking from sleep:
- Press and hold 3s:  Powers ON the device
- After power-on:     Shows an "Ojo del Sol ON" splash with logo for ~2 seconds
- Short press:        Shows "Hold 3s to power on" for 10 seconds
- No activity 10s:    Screen turns off, returns to sleep

GBA LINK PORT WIRING (OPTIONAL)
----------------------------------------------------------------------
This mode drives the GBA link port in General-Purpose SIO mode (4-bit parallel).
Pinout from GBATEK "Serial Link Port Pin-Out (GBA: EXT / GBA SP: EXT.1)":

| GBA Link Pin | Signal | XIAO ESP32S3 Pin | Notes |
| :----------- | :----- | :--------------- | :---- |
| 2            | SO     | D10 (GPIO10)     | bit 3 |
| 3            | SI     | D9 (GPIO8)       | bit 2 |
| 4            | SD     | D8 (GPIO7)       | bit 1 |
| 5            | SC     | D7 (GPIO44)      | bit 0 |
| 6            | GND    | GND              | common ground |
| 1            | VDD35  | (leave unconnected) | 3.3V from GBA, optional |

Notes:
- Only connect GND and the four signal lines above. Do not connect the GBA's
  VDD35 pin to 5V. Keep everything at 3.3V logic.
- GBA link port sockets/breakouts are sold separately; you can wire one directly
  to the XIAO pins listed above.
- This build uses the PCB from https://github.com/Palmr/gb-link-cable for the
  link cable connection.
- Use a proper GBA link cable (not an 8-bit Game Boy cable). Look for the
  mid-cable block/port found on official multi-play cables, and verify the plug
  has metal contacts for pins 2-6 (SO, SI, SD, SC, GND). Some cheap cables omit
  SO or SC and will not work.

GBA LINK OUTPUT MODE:
When GBA_LINK_ENABLED is true, the firmware outputs the current bar count as a
4-bit value on D7-D10.

3. LIBRARIES REQUIRED
----------------------------------------------------------------------
Install via Arduino Library Manager:
- Adafruit LTR390 Library
- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit BusIO
- NimBLE-Arduino (by h2zero)
- Callback (by Tom Stewart)

Install manually (download ZIP from GitHub):
- ESP32-BLE-CompositeHID: https://github.com/Mystfit/ESP32-BLE-CompositeHID

4. USAGE
----------------------------------------------------------------------
1. Power on the device (hold button 3 seconds)
2. Tap button to select your game (BOKTAI 1, 2, or 3)
3. Point the LTR390 sensor toward the sky/sun
4. Read the bar count from the display
5. Enter the matching sun level in-game using Prof9's ROM hacks
   (or your flash cart's input method)
6. Power off when done (hold button 3 seconds)

BLUETOOTH (mGBA / RetroArch):
When BLUETOOTH_ENABLED is true, the device presents as an Xbox Series X
controller named "Ojo del Sol Sensor" (change via BLE_DEVICE_NAME in config.h).
This provides proper XInput support on both PC and Android.

Pair it in your OS, then map L3/R3 in mGBA or RetroArch to control the meter:
- L3 (Left Stick click): decreases the in-game meter by 1
- R3 (Right Stick click): increases the in-game meter by 1

BLE control modes (set via BLE_CONTROL_MODE in config.h):
- Incremental Control Mode (default, BLE_CONTROL_MODE = 0): uses L3/R3 presses to step the meter.
- Chord Control Mode (BLE_CONTROL_MODE = 1): holds L3 + stick directions per bar amount.
  Chord mapping:
  0 bars: L3
  1 bar: Left stick up + L3
  2 bars: Left stick right + L3
  3 bars: Left stick down + L3
  4 bars: Left stick left + L3
  5 bars: Right stick up + L3
  6 bars: Right stick right + L3
  7 bars: Right stick down + L3
  8 bars: Right stick left + L3
  9 bars (Boktai 2/3 only): Both sticks up + L3
  10 bars (Boktai 2/3 only): Both sticks right + L3

In Incremental Control Mode, the firmware keeps the in-game meter synced with
L3/R3 presses as the sensor changes, and performs a clamp+refill resync every
BLE_RESYNC_INTERVAL_MS (disable via BLE_RESYNC_ENABLED or set the interval to 0).
Pairing stops after BLE_PAIRING_TIMEOUT_MS.
If a connection drops, the device re-enters pairing/advertising for the same
timeout window.
Press speed is set by BLE_BUTTONS_PER_SECOND. To use different buttons,
adjust BLE_BUTTON_DEC and BLE_BUTTON_INC in config.h using the Xbox button
constants from XboxGamepadDevice.h.
For Boktai 1, mGBA uses a 10-step input despite the 8-bar gauge; the
firmware accounts for this when syncing. If mGBA is fixed, set
BLE_BOKTAI1_MGBA_10_STEP_WORKAROUND = false in config.h.
Pairing restarts on the next wake.
The Bluetooth icon sits to the left of the battery percentage on the main
screen and screensaver, flashing while pairing and solid when connected.

SCREENSAVER:
When SCREENSAVER_ACTIVE is true, the display switches to a bouncing
"Ojo del Sol" after SCREENSAVER_TIME minutes with no button activity.
The logo appears above the text, with the battery indicator centered below.
Pressing the button wakes the screen and resets the timer. If you are
manually entering the solar meter via ROM hack or emulator, you may want
to turn off the screensaver (set SCREENSAVER_TIME = 0), but be aware this
will affect the OLED's lifespan.

Board Settings (Arduino IDE):
- Boards Manager URL: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
- Board package: "esp32" by Espressif Systems (version 2.0.8 or newer)
- Board: "XIAO_ESP32S3"
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

UV STABILITY (Optional)
-----------------------
If the bar display feels jumpy, you can tune filtering in config.h:
- UVI_SMOOTHING_ENABLED: applies exponential smoothing to the UV reading
- UVI_SMOOTHING_ALPHA: higher = faster response, lower = smoother
- BAR_HYSTERESIS: requires a small UV margin before changing bars

UVI CALCULATION
---------------
The firmware derives the UV divisor from the sensor's current gain and
resolution. If you change those settings, the UV Index scaling updates
automatically. The fallback divisor assumes gain 18 and 20-bit (400ms).

6. TECHNICAL NOTES
----------------------------------------------------------------------
Original Boktai Cartridge Sensor (from GBATEK):
- Uses photodiode with 8-bit ADC (digital-ramp converter)
- Values are INVERTED: 0xE8 = darkness, 0x50 = max gauge, 0x00 = extreme
- Accessed via GPIO at 0x80000C4-0x80000C8

LTR390 UV Sensor (per datasheet DS86-2015-0004):
- Reference sensitivity: 2300 counts/UVI at 18x gain, 400ms
- UV Index formula: UVI = raw x (18/gain) x (400/int_time) / 2300
- At our settings (18x gain, 20-bit/400ms): UVI = raw / 2300
- Peak response: 300-350nm (matches solar UV-A/UV-B)
- NOT inverted: higher values = more UV
- Configured for: Gain 18x, 20-bit/400ms resolution (500ms measurement rate)

LTR390 MODULE LED:
The LED on the Adafruit LTR390 breakout is a power indicator wired
directly to VIN. It is NOT controllable via software - it will be on
whenever the sensor is powered. If you wire LTR390 VIN to D3 (GPIO4)
and keep SENSOR_POWER_ENABLED true, the LED turns off during deep sleep.
You can also cut the LED trace on the back of the LTR390 to further extend
battery life.

OLED LOW-POWER SLEEP:
The firmware sends SSD1306 Display OFF (0xAE) and Charge Pump OFF
(0x8D, 0x10), then releases SDA/SCL to reduce back-powering during
deep sleep. Some OLED modules keep pull-ups on the bus, so the LTR390
LED may still glow faintly.

SENSOR POWER CONTROL (Default: Enabled):
The default firmware powers the LTR390 VCC from a GPIO to cut power during
deep sleep. This must be a 3.3V GPIO that can source a few mA. Do NOT power
the OLED from a GPIO. Wire LTR390 VIN to D3 (GPIO4) when
SENSOR_POWER_ENABLED is true. Set SENSOR_POWER_ENABLED = false to power
LTR390 from 3V3 instead.

IMPORTANT: Do not place the sensor behind glass or standard plastic!
Most glass blocks 90%+ of UV light. Use an open aperture, quartz glass,
or UV-transparent acrylic if an enclosure window is needed.

7. TROUBLESHOOTING
----------------------------------------------------------------------

UV SENSOR READS 0 OR VERY LOW:
1. Set DEBUG_SERIAL = true in config.h, then open Serial Monitor (115200 baud)
   to see raw sensor counts
   - At UVI 6, expect ~192 raw counts (6 x 32)
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

DEVICE POWERS OFF QUICKLY:
If BATTERY_CUTOFF_ENABLED is true, the device will sleep when voltage
stays below VOLT_CUTOFF. Lower the cutoff threshold or disable it.
If you don't use the voltage divider, set BATTERY_SENSE_ENABLED = false.

DEVICE WON'T WAKE FROM SLEEP:
1. Press and hold the button for 3 full seconds
2. If you release early, the "Hold 3s to power on" prompt shows for 10 seconds;
   press and hold again while the prompt is visible
3. The prompt auto-hides after 10 seconds of inactivity
4. If screen is off, press and hold again to power on

8. FUTURE PLANS
----------------------------------------------------------------------
- [ ] USB output for emulators
- [ ] Maybe Lunar Knights support?

9. CREDITS & LINKS
----------------------------------------------------------------------
- GBATEK Solar Sensor Documentation:
  https://problemkaputt.de/gbatek.htm
- Prof9's Boktai ROM Hacks:
  (link to Prof9's patches when available)
- Seeed XIAO ESP32S3:
  https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
- Adafruit LTR390:
  https://www.adafruit.com/product/4831
