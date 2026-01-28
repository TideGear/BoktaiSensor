I've done this all at my own expense.\
Please consider donating via...

<a href="https://ko-fi.com/TideGear"><img src="https://storage.ko-fi.com/cdn/kofi6.png" height="24"></a> or 
<a href="https://www.paypal.com/donate/?hosted_button_id=THMF458QTBCAL"><img src="https://i.postimg.cc/VvChGNyd/Pay-Pal.png" height="24"></a>

https://ko-fi.com/TideGear

https://www.paypal.com/donate/?hosted_button_id=THMF458QTBCAL

----------------------------------------------------------------------

**Important!: This project is all very new, experimental, lightly-tested, and subject to change.\
I'm looking for serious contributors to...**

**1. Help tune the default device settings to *exactly* match how all three games handle a range of sunlight.**\
**2. Add support via Chord Control Mode to their emulators.**\
**3. Test and fix the code if needed.**\
**4. Test the GBA link cable support.**\
**5. Create a cool 3D-printable case.**\
**6. You tell me!**

**...so please contact me if you are interested at TideGear @at@ gmail .dot. com !**

**Also contact me if you want to buy a pre-assembled device or kit!**

Photos of a caseless prototype: https://photos.app.goo.gl/2Yt2UpbPR9df76Lb9 \
(When assembled, it will likely be the size of a large smart watch.)

----------------------------------------------------------------------

Ojo del Sol - ESP32-S3 UV Meter for Boktai Games
======================================================================

A substitute solar sensor for playing the Boktai series (Boktai 1, 2, 3)
on flash carts or emulators. Restores "Kojima's Intent" by using real
sunlight (UV) instead of artificial light hacks.

**Supported Games:**
- Boktai 1: The Sun Is in Your Hand (8-bar gauge)
- Boktai 2: Solar Boy Django (10-bar gauge)
- Boktai 3: Sabata's Counterattack (10-bar gauge)

----------------------------------------------------------------------

## Quick Start

**Recommended setup:** Bluetooth Incremental Mode + mGBA libretro core (RetroArch)

1. Build the hardware (see [Hardware](#hardware) below)
2. Flash the firmware (see [Firmware Setup](#firmware-setup))
3. Pair the device as a Bluetooth controller in your OS
4. In mGBA (libretro), map **L3** to solar sensor decrease and **R3** to increase
5. Power on: hold button 3 seconds
6. Tap button to select game (BOKTAI 1, 2, or 3). (Optional) Tap to the DEBUG screen to view raw UV/battery values.
7. Point sensor toward the sky — the device syncs the in-game meter automatically
8. Power off: hold button 3 seconds

**Important:** Do not place the sensor behind glass or standard plastic — most glass blocks 90%+ of UV light!

----------------------------------------------------------------------

## Output Modes

The Ojo del Sol can output bar values in three ways. Choose based on your setup:

### 1. Display Only (Manual Entry)
Read the bar count from the OLED and enter it manually using Prof9's ROM hacks or your flash cart's input method.

### 2. Bluetooth (Emulators) — Recommended
The device appears as an Xbox Series X controller ("Ojo del Sol Sensor") and sends button/stick inputs to control the emulator's solar sensor.

**Two BLE control modes** (set via `BLE_CONTROL_MODE` in config.h):

| Mode | How it works | Emulator support |
|------|--------------|------------------|
| **Incremental (default, mode 0)** | Sends L3/R3 button presses to step the meter up/down | **Works now** with mGBA libretro core |
| **Chord (mode 1)** | Holds L3 + stick directions to encode bar value directly | **Not yet supported** by most emulators |

**Current recommendation:** Use **Incremental Mode** with the **mGBA libretro core** in RetroArch. This is the only tested, working configuration.

**Chord Mode status:** On January 23, 2026, I submitted a PR to the libretro mGBA core to add chord support (along with other solar tweaks). It may or may not be merged.

<details>
<summary>Chord mapping (for emulator developers)</summary>

- 0 bars: L3 only
- 1 bar: Left stick up + L3
- 2 bars: Left stick right + L3
- 3 bars: Left stick down + L3
- 4 bars: Left stick left + L3
- 5 bars: Right stick up + L3
- 6 bars: Right stick right + L3
- 7 bars: Right stick down + L3
- 8 bars: Right stick left + L3
- 9 bars (Boktai 2/3): Both sticks up + L3
- 10 bars (Boktai 2/3): Both sticks right + L3

</details>

### 3. GBA Link Cable (Real Hardware)

**Status: Untested.** This outputs a 4-bit bar count on GPIO pins for use with a physical GBA via link cable.

**Requirements:**
- Prof9's ROM hack from the **gba-link-gpio** branch: https://github.com/Prof9/Boktai-Solar-Sensor-Patches/tree/gba-link-gpio/Source
- Wire the Ojo del Sol to the **Player 2** side of the link cable

**Limitation:** Boktai's normal link-cable modes (e.g., multiplayer) are **not compatible** with this and may never be. To use multiplayer alongside the Ojo del Sol, you'll need an emulator with link support (I've contacted Pizza Boy A Pro and Linkboy devs — TBD).

----------------------------------------------------------------------

## Hardware

**Components:**
- Seeed XIAO ESP32S3 (built-in LiPo charging)
- Adafruit LTR390 UV Sensor (I2C)
- SSD1306 128x64 OLED Display (I2C)
- Tactile push button (power/game select)
- 3.7V LiPo battery (450mAh recommended)
- 2x 100K ohm resistors (for battery monitoring — optional but recommended)

### Wiring

| Component | Pin | XIAO ESP32S3 | Notes |
|-----------|-----|--------------|-------|
| OLED VIN | | 3V3 | 3.3V power |
| OLED GND | | GND | |
| LTR390 VIN | | D3 (GPIO4) | Sensor power (GPIO-controlled) |
| LTR390 GND | | GND | |
| OLED + LTR390 SDA | | D4 (GPIO5) | I2C data (shared bus) |
| OLED + LTR390 SCL | | D5 (GPIO6) | I2C clock (shared bus) |
| Button | Leg A | D0 (GPIO1) | Signal (internal pull-up) |
| Button | Leg B | GND | |
| Battery + | | BAT+ (back pad) | Farthest from USB-C |
| Battery - | | BAT- (back pad) | Closest to USB-C |

### Battery Monitoring (Optional)

The XIAO doesn't expose battery voltage directly. To enable battery percentage display, add a voltage divider:

```
BAT+ ----[100K]----+---- D1 (GPIO2)
                   |
                 [100K]
                   |
GND ---------------+
```

Without this, set `BATTERY_SENSE_ENABLED = false` in config.h to hide the battery indicator.

**Calibration:** If battery % is wrong at full charge, adjust `VOLT_DIVIDER_MULT` in config.h (increase if reading low, decrease if high).

### GBA Link Cable Wiring (Optional)

Only needed if using GBA Link output mode.

| GBA Link Pin | Signal | XIAO Pin |
|--------------|--------|----------|
| 2 | SO | D10 (GPIO10) |
| 3 | SI | D9 (GPIO8) |
| 4 | SD | D8 (GPIO7) |
| 5 | SC | D7 (GPIO44) |
| 6 | GND | GND |
| 1 | VDD35 | (leave unconnected) |

**Notes:**
- Keep all signals at 3.3V logic
- Most cheap GBA link cables are wired per [this sheet](https://docs.google.com/spreadsheets/d/19uAjLaDji9D3lIKIEdB9RHNQ_F-Fo1NnXY90uRH3dQA/edit?usp=sharing) — use the **Player 2** side
- This build uses the PCB from https://github.com/Palmr/gb-link-cable

**Cable quality warnings:**
- Do **not** use 8-bit Game Boy cables — they lack the required signals
- Official multi-play cables have a mid-cable block (signal splitter); cheap cables may lack this
- Check that pins 2-6 have metal contacts — some budget cables omit SO or SC lines entirely

----------------------------------------------------------------------

## Firmware Setup

### Libraries Required

**Install via Arduino Library Manager:**
- Adafruit LTR390 Library
- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit BusIO
- NimBLE-Arduino (by h2zero)
- Callback (by Tom Stewart)

**Install manually (download ZIP from GitHub):**
- ESP32-BLE-CompositeHID: https://github.com/Mystfit/ESP32-BLE-CompositeHID

### Board Settings (Arduino IDE)

- **Boards Manager URL:** `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- **Board package:** esp32 by Espressif Systems (2.0.8 or newer)
- **Board:** XIAO_ESP32S3
- **USB CDC On Boot:** Enabled (for serial debugging)

----------------------------------------------------------------------

## Usage

### Button Controls

**When device is ON:**
- **Tap:** Cycle screens (1 → 2 → 3 → DEBUG → 1...). Set `DEBUG_SCREEN_ENABLED = false` in config.h to remove the DEBUG screen.
- **Hold 3s:** Power OFF (deep sleep, ~10µA)
- If screensaver is active, first tap wakes the screen

**When waking from sleep:**
- **Hold 3s:** Power ON
- **Short press:** Shows "Hold 3s to power on" for 10 seconds, then returns to sleep

### Bluetooth Details

The device advertises as "Ojo del Sol Sensor" (configurable via `BLE_DEVICE_NAME`).

**Incremental Mode specifics:**
- The firmware tracks the in-game meter state and sends L3/R3 presses to sync it
- Performs a clamp+refill resync every `BLE_RESYNC_INTERVAL_MS` (default 60s)
- Press rate controlled by `BLE_BUTTONS_PER_SECOND` (default 20)
- For Boktai 1, mGBA uses 10 internal steps despite 8 visible bars — the firmware compensates (disable via `BLE_BOKTAI1_MGBA_10_STEP_WORKAROUND = false` if fixed)
- **Button remapping:** Change `BLE_BUTTON_DEC` and `BLE_BUTTON_INC` in config.h to use different buttons (see `XboxGamepadDevice.h` for available constants)

**Chord Mode specifics:**
- Updates immediately on bar change (no rate throttling)
- Re-asserts L3 on each bar change for reliability

**Pairing:**
- Times out after `BLE_PAIRING_TIMEOUT_MS` (default 60s)
- Re-enters pairing if connection drops
- **Pairing restarts on wake** — after waking from sleep, the device re-advertises for pairing
- Bluetooth icon flashes while pairing, solid when connected

### Screensaver

After `SCREENSAVER_TIME` minutes (default 3) of no button activity, the display shows a bouncing "Ojo del Sol" logo. Press any button to wake. Set `SCREENSAVER_TIME = 0` to disable (but this affects OLED lifespan).

----------------------------------------------------------------------

## Calibration

### Automatic Mode (Default)

Bar thresholds are calculated from two values in config.h:

```c
const bool AUTO_MODE = true;
const float AUTO_UV_MIN = 0.5;   // UV Index for 1 bar
const float AUTO_UV_MAX = 6.0;   // UV Index for full bars
```

Bars are distributed evenly across this range for all games. UV Index 6.0 is typical for a sunny day in temperate climates.

### Manual Mode

Set `AUTO_MODE = false` to use per-game threshold arrays (`BOKTAI_1_UV[8]`, `BOKTAI_2_UV[10]`, `BOKTAI_3_UV[10]`).

### Stability Tuning

If the bar display feels jumpy:
- `UVI_SMOOTHING_ENABLED`: Applies exponential smoothing (default: `false`; set `true` to smooth)
- `UVI_SMOOTHING_ALPHA`: Controls how much weight new readings get vs. the running average (default: `0.5`; higher = faster response, lower = smoother). Setting `UVI_SMOOTHING_ALPHA = 1.0` is the same as `UVI_SMOOTHING_ENABLED = false`.
- `BAR_HYSTERESIS_ENABLED`: Enables bar hysteresis (default: `false`). Setting `BAR_HYSTERESIS_ENABLED = false` is the same as `BAR_HYSTERESIS = 0.0`.
- `BAR_HYSTERESIS`: Requires UV margin before changing bars (default: `0.2`; only used when `BAR_HYSTERESIS_ENABLED = true`)

----------------------------------------------------------------------

## Technical Notes

### Original Boktai Cartridge Sensor (from GBATEK)
- Photodiode with 8-bit ADC
- Values **inverted**: 0xE8 = darkness, 0x50 = max gauge, 0x00 = extreme
- Accessed via GPIO at 0x80000C4-0x80000C8

### LTR390 UV Sensor
- Reference sensitivity: 2300 counts/UVI at 18x gain, 400ms integration
- Formula: `UVI = raw / 2300` (at our settings: gain 18x, 20-bit/400ms, 500ms rate)
- Peak response: 300-350nm (UV-A/UV-B)
- **Not inverted**: higher values = more UV

### Power Management
- Sensor power can be GPIO-controlled (`SENSOR_POWER_PIN`) to cut power during sleep
- Alternatively, set `SENSOR_POWER_ENABLED = false` in config.h and wire LTR390 VIN to 3V3 instead of a GPIO pin
- OLED uses Display OFF + Charge Pump OFF commands
- Deep sleep current: ~10µA (varies with module pull-ups)
- The LTR390 breakout LED is hardwired to VIN — it turns off in sleep if you use GPIO power control

### UV Blocking Warning
Do not place the sensor behind glass or standard plastic — most glass blocks 90%+ of UV. Use an open aperture, quartz glass, or UV-transparent acrylic if an enclosure window is needed.

----------------------------------------------------------------------

## Troubleshooting

### UV sensor reads 0 or very low
1. Enable `DEBUG_SERIAL = true` and check Serial Monitor (115200 baud)
   - At UVI 6, expect ~13800 raw counts (6 × 2300)
   - If raw = 0, sensor may not be in UV mode
2. Ensure sensor faces the sky (not blocked by hand/case)
3. No glass/plastic in front of sensor
4. Indoor UV is near zero — test outside

### Battery shows 0%
You need the voltage divider circuit. Without it, set `BATTERY_SENSE_ENABLED = false`.

### Battery % is wrong
Adjust `VOLT_DIVIDER_MULT` in config.h (increase if reading low, decrease if high).

### Device powers off quickly
Check `BATTERY_CUTOFF_ENABLED` and `VOLT_CUTOFF`. Disable cutoff if not using the voltage divider.

### Device won't wake
1. Hold button for full 3 seconds
2. If you release early, the "Hold 3s to power on" prompt appears for 10 seconds
3. After 10s of no activity, it returns to sleep

----------------------------------------------------------------------

## Future Plans

- [ ] USB output for emulators
- [ ] Lunar Knights support?

----------------------------------------------------------------------

## Credits & Links

- [GBATEK Solar Sensor Documentation](https://problemkaputt.de/gbatek.htm)
- [Prof9's Boktai ROM Hacks](https://github.com/Prof9/Boktai-Solar-Sensor-Patches)
- [Seeed XIAO ESP32S3 Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [Adafruit LTR390](https://www.adafruit.com/product/4831)
