I've done this all at my own expense.\
Please consider donating via...

<a href="https://ko-fi.com/TideGear"><img src="https://storage.ko-fi.com/cdn/kofi6.png" height="24"></a> or 
<a href="https://www.paypal.com/donate/?hosted_button_id=THMF458QTBCAL"><img src="https://i.postimg.cc/VvChGNyd/Pay-Pal.png" height="24"></a>

https://ko-fi.com/TideGear

https://www.paypal.com/donate/?hosted_button_id=THMF458QTBCAL

----------------------------------------------------------------------

**Important!: This project is all very new, experimental, lightly-tested, and subject to change.\
I'm looking for serious contributors to...**

**1. Help tune the default device settings to *exactly* match how all three games handle a range of sunlight. Until I can do some testing, they are just guesses.**\
**2. Add support via Single Analog Control Mode to their emulators.**\
**3. Test and fix the code if needed.**\
**4. Test the GBA link cable support.**\
**5. Create a cool 3D-printable case.**\
**6. You tell me!**

**...so please contact me if you are interested at TideGear @at@ gmail .dot. com !**

**Also contact me if you want to buy a pre-assembled device or kit!**

Photos of a unfinished prototype: https://photos.app.goo.gl/ex75rCqATgqSrdCb6

----------------------------------------------------------------------

Ojo del Sol - ESP32-S3 UV Meter for Boktai Games
======================================================================

A substitute solar sensor for playing the Boktai series (Boktai 1, 2, 3)
on flash carts or emulators. Restores "Kojima's Intent" by using real
sunlight (UV) instead of artificial light hacks.

### Based on Prof9's Work

The GBA link cable ASM patches, IPS patches, and Ojo-del-Sol-side GBA link
protocol are based on [the gba-link-gpio branch of Prof9's Boktai Solar Sensor Patches](https://github.com/Prof9/Boktai-Solar-Sensor-Patches/tree/gba-link-gpio/Source).
Prof9's original code is released under the MIT License — see
[Prof9's license.txt](Prof9's%20license.txt) for details.

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
6. Tap button to select game (BOKTAI 1, 2, or 3). (Optional) Tap to the DEBUG screen to view UV raw, UVI, compensated UVI, ADC avg, and battery voltage.
7. Point sensor toward the sky — the device syncs the in-game meter automatically
8. Power off: hold button 3 seconds

**Important:** Only wake the device once you are in-game. The controller inputs it sends (button presses, stick deflections) may cause unwanted behavior in menus or other apps. When possible, put the device to sleep (hold 3s) when you are not actively playing a Boktai game.

**Important:** Placing the sensor behind standard glass or plastic blocks UV light. Use the `UV_ENCLOSURE_*` settings in `config.h` to compensate for your enclosure window. Strongly UV-blocking materials can still reduce usable range. The default values will be (but not yet) set to adjust for the transmission loss of these boxes: https://a.co/d/092sVvLT

----------------------------------------------------------------------

## Output Modes

The Ojo del Sol can output bar values in three ways. Choose based on your setup:

### 1. Display Only (Manual Entry)
Read the bar count from the OLED and enter it manually using Prof9's ROM hacks (https://github.com/Prof9/Boktai-Solar-Sensor-Patches). If you intend to use manual entry, **do not** use the ROM hacks in the GBA Link Patches directory. Those are for the GBA link cable method below.

### 2. Bluetooth (Emulators) — Recommended
The device appears as an Xbox Series X controller ("Ojo del Sol Sensor") and sends button/stick inputs to control the emulator's solar sensor.

**Two BLE control modes** (set via `BLE_CONTROL_MODE` in config.h):

| Mode | How it works | Emulator support |
|------|--------------|------------------|
| **Incremental (default, mode 0)** | Sends L3/R3 button presses to step the meter up/down | **Works now** with mGBA libretro core |
| **Single Analog (mode 1)** | Better for Ojo del Sol! Maps bar count to proportional deflection on one analog axis | Requires emulator support |

**Current recommendation:** Use **Incremental Mode** with the **mGBA libretro core** in RetroArch until Single Analog Mode is supported by emulators.

**Emulator developers:** See [For Emulator Devs.md](For%20Emulator%20Devs.md) for the Single Analog Mode specification, band mapping tables, and pseudocode.

### 3. GBA Link Cable (Real Hardware)

**Status: Partially Tested.** This outputs a framed 3-wire value (SC + SD + SO), using the SI conductor as a ground bridge for use with a physical GBA via link cable.

**Requirements:**
- Use the updated ROM hack IPS patches in this repo's `GBA Link Patches/` folder (`Source/` contains the ASM sources). Many thanks to Prof9 for the original proof-of-concept code!
- Wire the Ojo del Sol to the **Player 1 (P1)** side of the cheap cable.
- Wire the GBA to the **Player 2 (P2)** side.

**Note:** The English translation of Boktai 3 should work with the patch, but this is currently untested.

**Limitation:** Boktai's normal link-cable modes (e.g., multiplayer) are **not compatible** with this and may never be. To use multiplayer alongside the Ojo del Sol, you'll need an emulator with link support (I've contacted Pizza Boy A Pro and Linkboy devs — TBD).

----------------------------------------------------------------------

## Hardware

**Components:**
- Seeed XIAO ESP32S3 (built-in LiPo charging)
- Adafruit LTR390 UV Sensor (I2C)
- SSD1306 128x64 OLED Display (I2C)
- Tactile push button (power/game select)
- 3.7V LiPo battery (300mAh works well; larger capacity is optional but be mindful of your case size)
- 2x 100K ohm resistors (for battery monitoring — optional but recommended)

### Wiring

| Component | Pin | XIAO ESP32S3 | Notes |
|-----------|-----|--------------|-------|
| OLED VIN | | 3V3 | 3.3V power |
| OLED GND | | GND | |
| LTR390 VIN | | D3 (GPIO4) | Sensor power (GPIO-controlled) |
| LTR390 GND | | GND | |
| OLED + LTR390 SDA | | D4 (GPIO5) | I2C data (configurable via `I2C_SDA_PIN`) |
| OLED + LTR390 SCL | | D5 (GPIO6) | I2C clock (configurable via `I2C_SCL_PIN`) |
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

Battery sensing is disabled by default (`BATTERY_SENSE_ENABLED = false`). After wiring the divider, set `BATTERY_SENSE_ENABLED = true` in `config.h` to show battery data.

**Calibration:** If battery % is wrong at full charge, adjust `VOLT_DIVIDER_MULT` in config.h (increase if reading low, decrease if high).

### GBA Link Cable Wiring (Optional)

Only needed if using GBA Link output mode. For the framed 3-wire protocol, the GBA connects to the **Player 2 (P2)** side of a cheap cable. The Ojo del Sol connects to the **Player 1 (P1)** side via a [Palmr breakout board](https://github.com/Palmr/gb-link-cable).

**Framed 3-wire signal mapping:**

| Function | XIAO Pin | Notes |
|----------|----------|-------|
| Frame phase (SC) | D7 (GPIO44) | Phase bit toggles every `GBA_LINK_FRAME_TOGGLE_MS` |
| Payload bit 1 (SD) | D8 (GPIO7) | Framed data |
| Payload bit 0 (SO) | D9 (GPIO8) | Framed data |
| Ground bridge wire | Ojo GND | Connect to P1 SI pad (Pin 3); this routes to GBA ground on P2 Pin 6 |
| VDD35 | - | Leave unconnected |

**Cheap cable continuity and P1-side wiring:**

Cheap GBA link cables have non-standard internal wiring (no mid-cable signal splitter), and the two ends are not equivalent. For this method, wire the **P1 breakout** as follows:

| Ojo Connection | P1 Breakout Pad Label | P1 Pin | Cable Routes To | P2 (GBA) Pin |
|----------------|------------------------|--------|-----------------|--------------|
| D9 (GPIO8) | SO | Pin 2 | P2 Pin 3 | SI |
| D8 (GPIO7) | SD | Pin 4 | P2 Pin 4 | SD |
| D7 (GPIO44) | SC | Pin 5 | P2 Pin 5 | SC |
| GND | SI | Pin 3 | P2 Pin 6 | GND |

This intentionally repurposes the P1 `SI` conductor as the shared ground path for the Ojo and GBA.

Verify your specific cable's wiring with a multimeter continuity test. See [this sheet](https://docs.google.com/spreadsheets/d/19uAjLaDji9D3lIKIEdB9RHNQ_F-Fo1NnXY90uRH3dQA/edit?usp=sharing) for reference.

Expected continuity on this cheap-cable variant:
- P1 Pin 2 -> P2 Pin 3
- P1 Pin 3 -> P2 Pin 6
- P1 Pin 4 -> P2 Pin 4
- P1 Pin 5 -> P2 Pin 5

**Important - shield and "GND" labels:**

- Shell/shield continuity is often missing on cheap cables, so do not assume shield gives ground reference.
- On this wiring method, shared ground comes from the P1 Pin 3 conductor routed to P2 Pin 6.

**`GBA_LINK_FRAME_TOGGLE_MS` explained:**

- This value is the hold time for one frame phase (`SC` low or high).
- One full 4-bit bar value needs two phases, so full refresh time is roughly `2 * GBA_LINK_FRAME_TOGGLE_MS`.
- Example: `5ms` means about `10ms` per full value (~100 full updates/sec).
- Lower values respond faster but reduce electrical timing margin; higher values are slower but more tolerant.

**Notes:**
- Keep all signals at 3.3V logic (the XIAO ESP32S3 is 3.3V native, so no level shifting is needed)


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

**Install manually (included in repo as ESP32-BLE-CompositeHID-master.zip, or download from GitHub):**
- ESP32-BLE-CompositeHID: https://github.com/Mystfit/ESP32-BLE-CompositeHID

### Board Settings (Arduino IDE)

- **Boards Manager URL:** `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- **Board package:** esp32 by Espressif Systems
- **Recommended version:** `3.3.6`
- **Known issue:** `3.3.7` may crash during BLE startup with:
  `assert failed: block_locate_free tlsf_control_functions.h:618`
  If this happens, roll back to `3.3.6` in Boards Manager.
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
- **Short press/tap:** Immediately shows "Hold 3s to power on"
- If there is no button activity for 10 seconds, it returns to sleep

### Bluetooth Details

The device advertises as "Ojo del Sol Sensor" (configurable via `BLE_DEVICE_NAME`).

**Incremental Mode specifics:**
- The firmware tracks the in-game meter state and sends L3/R3 presses to sync it
- Performs a clamp+refill resync every `BLE_RESYNC_INTERVAL_MS` (default 60s)
- Press rate controlled by `BLE_BUTTONS_PER_SECOND` (default 20)
- For Boktai 1, mGBA uses 10 internal steps despite 8 visible bars — the firmware compensates (disable via `BLE_BOKTAI1_MGBA_10_STEP_WORKAROUND = false` if fixed)
- **Button remapping:** Change `BLE_BUTTON_DEC` and `BLE_BUTTON_INC` in config.h to use different buttons (see `XboxGamepadDevice.h` for available constants)

**Single Analog Mode specifics:**
- Updates immediately on bar change (no rate throttling)
- Maps bar count to a proportional deflection on a configurable analog axis (default: Right X+)
- Optionally holds a configurable "meter unlock" button (default: R3) while the axis is active (`BLE_METER_UNLOCK_BUTTON_ENABLED`). Single Analog mode uses midpoint band mapping, so even 0 bars sends a small deflection (not exact center). The button is released and re-pressed on every deflection change and periodically (`BLE_METER_UNLOCK_REFRESH_MS`, default 1000ms) to ensure apps/emulators always register it.
- Axis and unlock button are configurable in config.h (`BLE_SINGLE_ANALOG_AXIS`, `BLE_METER_UNLOCK_BUTTON`)

**Pairing:**
- Times out after `BLE_PAIRING_TIMEOUT_MS` (default 60s)
- Re-enters pairing if connection drops
- **Pairing restarts on wake** — after waking from sleep, the device re-advertises for pairing
- Bluetooth icon flashes while pairing, solid when connected

### Screensaver

After `SCREENSAVER_TIME` minutes (default 3) of no button activity, the display shows a bouncing "Ojo del Sol" logo. Press any button to wake. Set `SCREENSAVER_TIME = 0` to disable (but this affects OLED lifespan). The status row appears only when battery data is available and/or BLE is actively connected or pairing.

----------------------------------------------------------------------

## Calibration

### Automatic Mode (Default)

Bar thresholds are calculated from two values in config.h:

```c
const bool AUTO_MODE = true;
const float AUTO_UV_MIN = 0.500;   // UV Index for 1 bar
const float AUTO_UV_SATURATION = 6.000;   // UV Index ceiling (output clamp)
```

Bars are distributed evenly across this range for all games. `AUTO_UV_SATURATION` is a saturation ceiling, so the highest bar can begin before this value and then remains clamped at full above it. UV Index 6 is typical for a sunny day in temperate climates.

### Manual Mode

Set `AUTO_MODE = false` to use per-game threshold arrays (`BOKTAI_1_UV[8]`, `BOKTAI_2_UV[10]`, `BOKTAI_3_UV[10]`).

### Transparent Enclosure Compensation

If the sensor is behind a transparent window, model that window as transmission loss:

```c
const bool UV_ENCLOSURE_COMP_ENABLED = true;
const float UV_ENCLOSURE_TRANSMITTANCE = 0.42; // 42% UV passes through the window
const float UV_ENCLOSURE_UVI_OFFSET = 0.000;     // Optional bias correction
```

Firmware applies compensation once after raw-to-UVI conversion:

`corrected_uvi = max(0, (measured_uvi - offset) / transmittance)`

Quick calibration:
1. Temporarily set `UV_ENCLOSURE_COMP_ENABLED = false` while collecting calibration readings.
2. Take side-by-side readings at the same time/angle: one sensor open-air, one in-box.
3. Compute `in_box_uvi / open_air_uvi` across several samples.
4. Use the median ratio as `UV_ENCLOSURE_TRANSMITTANCE`.
5. Re-enable compensation (`UV_ENCLOSURE_COMP_ENABLED = true`).

Note: On the DEBUG screen, `UVI` is the measured (pre-compensation) value and `UVI comp'ed` is the post-compensation value.

### Stability Tuning

If the bar display feels jumpy:
- `UVI_SMOOTHING_ENABLED`: Applies exponential smoothing (default: `false`; set `true` to smooth)
- `UVI_SMOOTHING_ALPHA`: Controls how much weight new readings get vs. the running average (default: `0.5`; higher = faster response, lower = smoother). Setting `UVI_SMOOTHING_ALPHA = 1.0` is the same as `UVI_SMOOTHING_ENABLED = false`.
- `BAR_HYSTERESIS_ENABLED`: Enables bar hysteresis (default: `false`). Setting `BAR_HYSTERESIS_ENABLED = false` is the same as `BAR_HYSTERESIS = 0.0`.
- `BAR_HYSTERESIS`: Requires UV margin before changing bars (default: `0.200`; only used when `BAR_HYSTERESIS_ENABLED = true`)

----------------------------------------------------------------------

## Technical Notes

### Original Boktai Cartridge Sensor (from GBATEK)
- Photodiode with 8-bit ADC
- Values **inverted**: 0xE8 = darkness, 0x50 = max gauge, 0x00 = extreme
- Accessed via GPIO at 0x80000C4-0x80000C8

### LTR390 UV Sensor
- Reference sensitivity: 2300 counts/UVI at 18x gain, 400ms integration
- Formula (measured): `UVI = raw / 2300` (at our settings: gain 18x, 20-bit/400ms, 500ms rate)
- Formula (corrected, optional enclosure compensation): `UVI = max(0, (measured_uvi - offset) / transmittance)`
- Peak response: 300-350nm (UV-A/UV-B)
- **Not inverted**: higher values = more UV

### Power Management
- Sensor power can be GPIO-controlled (`SENSOR_POWER_PIN`) to cut power during sleep
- Alternatively, set `SENSOR_POWER_ENABLED = false` in config.h and wire LTR390 VIN to 3V3 instead of a GPIO pin
- Battery sensing and low-voltage cutoff are disabled by default; enable after wiring and validating the battery divider (`BATTERY_SENSE_ENABLED`, `BATTERY_CUTOFF_ENABLED`)
- Battery sampling/cutoff checks run on a timed loop and are not dependent on UV "new data" events
- OLED uses Display OFF + Charge Pump OFF commands
- On wake, firmware explicitly re-enables the OLED charge pump/display before drawing
- Deep sleep current: ~10µA (varies with module pull-ups)
- The LTR390 breakout LED is hardwired to VIN — it turns off in sleep if you use GPIO power control

### UV Blocking Warning
Most glass and many plastics block UV strongly (often 90%+). Compensation can correct scale loss, but it cannot recover signal if too little UV reaches the sensor. Prefer an open aperture, quartz glass, or UV-transparent acrylic.

----------------------------------------------------------------------

## Troubleshooting

### UV sensor reads 0 or very low
1. Enable `DEBUG_SERIAL = true` and check Serial Monitor (115200 baud)
   - At UVI 6, expect ~13800 raw counts (6 × 2300)
   - If raw = 0, sensor may not be in UV mode
2. Ensure sensor faces the sky (not blocked by hand/case)
3. If using an enclosure window, verify `UV_ENCLOSURE_TRANSMITTANCE` and use UV-transparent material
4. Indoor UV is near zero — test outside

### Battery shows 0%
You need the voltage divider circuit and `BATTERY_SENSE_ENABLED = true`. Keep battery sensing disabled until the divider is wired.

### Battery % is wrong
Adjust `VOLT_DIVIDER_MULT` in config.h (increase if reading low, decrease if high).

### Device powers off quickly
Check `BATTERY_CUTOFF_ENABLED` and `VOLT_CUTOFF`. Cutoff is disabled by default; only enable it after battery sensing is wired and validated.

### Device won't wake
1. Hold button for full 3 seconds
2. A short press should immediately show the "Hold 3s to power on" prompt
3. After 10s of no activity, it returns to sleep
4. If the OLED fails to initialize, the device enters deep sleep after about 2 seconds using the normal sleep path (same cleanup and button-release debounce as manual sleep). Check I2C wiring and `DISPLAY_I2C_ADDR` in config.h (some modules use `0x3D` instead of `0x3C`)

### BLE crashes at startup (`block_locate_free` TLSF assert)
If Serial Monitor shows:
`assert failed: block_locate_free tlsf_control_functions.h:618`
right after BLE/BTDM startup logs, this is a known board-package compatibility issue on some setups with esp32 core `3.3.7`.

**Fix:** In Arduino IDE -> Boards Manager, install esp32 core `3.3.6` (or the last known-good version for your environment), then rebuild and flash.

----------------------------------------------------------------------

## Future Plans

- [ ] USB output for emulators
- [ ] Lunar Knights support?

----------------------------------------------------------------------

## Credits & Links

- **[Prof9](https://github.com/Prof9/Boktai-Solar-Sensor-Patches)** — GBA link patches, IPS patches, and link protocol are based on Prof9's work (MIT License — see [Prof9's license.txt](Prof9's%20license.txt)) Prof9 also provided a ridiculous amount of help, patience, and kindness. Many thanks!
- [GBATEK Solar Sensor Documentation](https://problemkaputt.de/gbatek.htm)
- [Seeed XIAO ESP32S3 Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [Adafruit LTR390](https://www.adafruit.com/product/4831)
