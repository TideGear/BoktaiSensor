I've done this all at my own expense.\
Please consider donating via...

<a href="https://ko-fi.com/TideGear"><img src="https://storage.ko-fi.com/cdn/kofi6.png" height="24"></a> or 
<a href="https://www.paypal.com/donate/?hosted_button_id=THMF458QTBCAL"><img src="https://i.postimg.cc/VvChGNyd/Pay-Pal.png" height="24"></a>

https://ko-fi.com/TideGear

https://www.paypal.com/donate/?hosted_button_id=THMF458QTBCAL

----------------------------------------------------------------------

**Important!: This project is all very new, experimental, lightly-tested, and subject to change.\
I'm looking for serious contributors to...**

**1. Help tune the default device settings to *exactly* match how all three games handle a range of sunlight.** The defaults are roughly calibrated against real sunlight and a real Boktai 3 cartridge, but more testing across conditions and games is welcome. Where testing is ambiguous (e.g. flickering meter bar in the game), I tend to skew toward whatever is more "challenging" to achieve (seeking out more sunlight). However, it's usually a matter of fractions of a UVI.\
**2. Add support via Single Analog Control Mode to their emulators.**\
**3. Test and fix the code if needed.**\
**4. Create a cool 3D-printable case.**\
**5. You tell me!**

**...so please contact me if you are interested at TideGear @at@ gmail .dot. com !**

**Also contact me if you want to buy a pre-assembled device or kit!**

Photos of a prototype: https://photos.app.goo.gl/sYk1wwQbzPgmbmyS8 \
Made with this 1.7 x 1.7 x 0.79 inch case: https://a.co/d/01qMPdr9

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

**Recommended setup:** Incremental Mode (`HID_CONTROL_MODE = 0`) with mGBA libretro core (RetroArch), over either Bluetooth or USB XInput

1. Build the hardware (see [Hardware](#hardware) below)
2. Flash the firmware (see [Firmware Setup](#firmware-setup))
3. For mGBA libretro, set `HID_CONTROL_MODE = 0` in `config.h`
4. Pair the device as a Bluetooth controller in your OS
5. In mGBA (libretro), map **L3** to solar sensor decrease and **R3** to increase
6. Power on: hold button 2 seconds
7. Tap button to select game (BOKTAI 1, 2, or 3). (Optional) Tap to the **XInput** screen to view UV raw, UVI, compensated UVI, ADC avg, and battery voltage.
8. Point sensor toward the sky — the device syncs the in-game meter automatically
9. Hold button 2 seconds on a game screen to sleep. On the **XInput** screen, hold 2 seconds to enter CDC mode (USB serial / firmware upload).

**Important:** Only wake the device once you are in-game. The controller inputs it sends (button presses, stick deflections) may cause unwanted behavior in menus or other apps. When possible, put the device to sleep (hold 2s) when you are not actively playing a Boktai game.

**Important:** Placing the sensor behind standard glass or plastic blocks UV light. Use the `UV_ENCLOSURE_*` settings in `config.h` to compensate for your enclosure window. Strongly UV-blocking materials can still reduce usable range. The default transmittance (0.833) is currently a very quick and dirty calibration for these boxes: https://a.co/d/092sVvLT — adjust if you use a different enclosure.

----------------------------------------------------------------------

## Output Modes

The Ojo del Sol can output bar values in four ways. Choose based on your setup:

### 1. Bluetooth (Recommended for Emulators on PC or Mobile)
The device appears as an Xbox Series X controller ("Ojo del Sol Sensor") and sends button/stick inputs to control the emulator's solar sensor.

**Two HID control modes** (shared by Bluetooth and USB XInput; set via `HID_CONTROL_MODE` in config.h):

| Mode | How it works | Emulator support |
|------|--------------|------------------|
| **Incremental (default, mode 0)** | Sends L3/R3 button presses to step the meter up/down | **Works now** with mGBA libretro core |
| **Single Analog (mode 1)** | Better for Ojo del Sol! Maps bar count to proportional deflection on one analog axis | Requires emulator support |

**Current recommendation:** Use **Incremental Mode** with the **mGBA libretro core** in RetroArch until Single Analog Mode is supported by emulators (transport can be Bluetooth or USB XInput).

**Emulator developers:** See [For Emulator Devs.md](For%20Emulator%20Devs.md) for the Single Analog Mode specification, band mapping tables, and pseudocode.

### 2. USB XInput (Also Great for Emulators on PC or Mobile)
When `USB_HID_ENABLED = true`, normal boot enumerates as an Xbox 360-compatible USB XInput device (product string: `Ojo del Sol`) for emulator/game compatibility.
When `USB_HID_ENABLED = false`, the device automatically enters CDC mode on boot (one brief restart on first power-on or after a full power cycle; subsequent wakes return directly to CDC).
Both HID control modes are available over USB:
- **Incremental (`HID_CONTROL_MODE = 0`)**: sends `HID_BUTTON_DEC`/`HID_BUTTON_INC` step inputs and works independently of BLE state.
- **Single Analog (`HID_CONTROL_MODE = 1`)**: sends the configured axis plus optional unlock button (`HID_SINGLE_ANALOG_AXIS`, `HID_METER_UNLOCK_*`).

USB CDC serial is not active during normal XInput runtime. To enable USB serial (for `DEBUG_SERIAL` output or firmware upload):
1. Tap to the **XInput** screen (last screen in the cycle; this diagnostics screen is always present).
2. Hold the button for 2 seconds.
3. The device restarts into CDC mode and enumerates as `Ojo del Sol (CDC)`.

With `DEBUG_SERIAL = true`, CDC output includes startup diagnostics, device-button press events, and any enabled UV/battery debug streams.

In CDC mode the firmware runs fully (UV sensing, game display, screensaver, BLE, GBA link). The screen header shows **CDC** instead of **XInput**. Hold the button for 2 seconds from any screen to exit CDC mode and sleep; on next wake, the device boots back into XInput mode.

As an alternative, set `USB_HID_ENABLED = false` to have the firmware boot directly into CDC on wake.

### 3. GBA Link Cable (Best Option for Flash Carts)

This outputs a framed 3-wire value (SC + SD + SO) for use with a physical GBA via link cable.

**Requirements:**
- Use the updated ROM hack IPS patches in this repo's `GBA Link Patches/` folder (`Source/` contains the ASM sources). Many thanks to Prof9 for the original proof-of-concept code!
- Recommended: install a real GBA link port on the Ojo del Sol and wire it as shown in [GBA Link Port Wiring.png](GBA%20Link%20Port%20Wiring.png). Link ports can be ordered here: https://a.co/d/00r3xzuj
- A breakout board also works, but the link cable can come loose easily if you move around much, so the real link port is the more robust option.
- Connect the Ojo del Sol to the **Player 1 (P1)** side of the cable and the GBA to the **Player 2 (P2)** side.

**Note:** The English translation of Boktai 3 should work with the patch, but this is currently untested.

**Limitation:** Boktai's normal link-cable modes (e.g., multiplayer) are **not compatible** with this and may never be. To use multiplayer alongside the Ojo del Sol, you'll need an emulator with link support (I've contacted Pizza Boy A Pro and Linkboy devs — TBD).

### 4. Display Only / Manual Entry (Not Recommended)
Read the bar count from the OLED and enter it manually using Prof9's ROM hacks (https://github.com/Prof9/Boktai-Solar-Sensor-Patches). If you intend to use manual entry, **do not** use the ROM hacks in the GBA Link Patches directory. Those are for the GBA link cable method above.

**Note:** The screen is difficult to read in direct sunlight unless you shade it, so the display is really more for calibration and occasional reference.

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
| LTR390 VIN | | 3V3 | 3.3V power |
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

Only needed if using GBA Link output mode. Recommended: install a real GBA link port on the Ojo del Sol and wire it as shown in [GBA Link Port Wiring.png](GBA%20Link%20Port%20Wiring.png). Link ports can be ordered here: https://a.co/d/00r3xzuj

A breakout board such as the [Palmr breakout board](https://github.com/Palmr/gb-link-cable) still works, but the link cable can come loose easily if you intend to move around much. An actual link port is a much more robust solution.

**Recommended Ojo-side link port wiring:**

| Ojo Connection | Ojo-side GBA Port Pin Type | Equivalent P1 Breakout Pad | Notes |
|----------------|----------------------------|----------------------------|-------|
| D9 (GPIO8) | SI | SO | Payload bit 0 |
| GND | GND | SI | Shared ground |
| D8 (GPIO7) | SD | SD | Payload bit 1 |
| D7 (GPIO44) | SC | SC | Frame phase; toggles every `GBA_LINK_FRAME_TOGGLE_MS` |

Leave the Ojo-side port `SO` and `VDD35` contacts unconnected. If you use a breakout board instead of a port, wire the equivalent P1 breakout pads listed above.

**Cheap cable continuity and cable orientation:**

Cheap GBA link cables have non-standard internal wiring (no mid-cable signal splitter), and the two ends are not equivalent. Plug the Ojo del Sol into the **Player 1 (P1)** side of the cable and the GBA into the **Player 2 (P2)** side.

Verify your specific cable's wiring with a multimeter continuity test. See [this sheet](https://docs.google.com/spreadsheets/d/19uAjLaDji9D3lIKIEdB9RHNQ_F-Fo1NnXY90uRH3dQA/edit?usp=sharing) for reference.

Expected continuity on this cheap-cable variant:
- P1 Pin 2 -> P2 Pin 3
- P1 Pin 3 -> P2 Pin 6
- P1 Pin 4 -> P2 Pin 4
- P1 Pin 5 -> P2 Pin 5

This wiring intentionally uses the conductor that routes P1 Pin 3 to P2 Pin 6 as the shared ground path between the Ojo and GBA.

**Important - shield and "GND" labels:**

- Shell/shield continuity is often missing on cheap cables, so do not assume shield gives ground reference.
- On this wiring method, shared ground comes from the P1 Pin 3 conductor routed to P2 Pin 6.

**`GBA_LINK_FRAME_TOGGLE_MS` explained:**

- This value is the hold time for one frame phase (`SC` low or high).
- One full 4-bit bar value needs two phases, so ideal full refresh time is roughly `2 * GBA_LINK_FRAME_TOGGLE_MS`.
- Firmware schedules phase timing with a microsecond clock, so `GBA_LINK_FRAME_TOGGLE_MS` is now honored much more closely than simple loop-delay timing.
- Example: `5ms` targets about `10ms` per full value (~100 full updates/sec), subject to normal loop jitter.
- Lower values respond faster but reduce electrical timing margin; higher values are slower but more tolerant.

**Notes:**
- Keep all signals at 3.3V logic (the XIAO ESP32S3 is 3.3V native, so no level shifting is needed)
- The ESP32 writes data pins (SD, SO) before the phase pin (SC) so the GBA always samples stable data when it detects a phase edge. The GBA-side ASM patches include a consecutive-match check that discards any single-frame misread, adding one frame (~17ms) of latency on real bar changes but eliminating visible glitches.


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
- **Recommended version:** `3.3.7`
- **Board:** XIAO_ESP32S3
- **USB Mode:** USB-OTG (TinyUSB)
- **USB CDC On Boot:** Enabled (required for CDC mode; normal runtime remains XInput unless you enter CDC mode from the XInput screen or set `USB_HID_ENABLED = false`)
- **Upload Mode:** USB-OTG CDC (TinyUSB) (required for CDC-mode uploads; UART0/Hardware CDC will fail to auto-connect in this setup)
- If CDC UI appears inconsistent/garbled after failed uploads, do one upload with **Erase All Flash Before Sketch Upload = Enabled**, then set it back to Disabled.

----------------------------------------------------------------------

## Usage

### Button Controls

**When device is ON:**
- **Tap:** Cycle screens (BOKTAI 1 → 2 → 3 → XInput → 1...). The XInput screen is always present and doubles as the diagnostics screen.
- **Hold 2s on a game screen:** Power OFF (deep sleep, ~10uA)
- **Hold 2s on the XInput screen:** Restart into CDC mode for USB serial output and firmware upload
- **Hold 2s while in CDC mode:** Exit CDC mode and sleep (next wake returns to XInput mode)
- If screensaver is active, first tap wakes the screen

**When waking from sleep:**
- **Hold 2s:** Power ON
- **Short press/tap:** Immediately shows "Hold 2s to power on"
- If there is no button activity for 10 seconds, it returns to sleep

### HID Control Mode Details (Bluetooth + USB)

**Incremental Mode specifics:**
- Works over both Bluetooth and USB XInput.
- The firmware tracks the in-game meter state and sends L3/R3 presses to sync it
- On BLE connections, performs a clamp+refill resync every `BLE_RESYNC_INTERVAL_MS` (default 60s)
- Press rate controlled by `HID_BUTTONS_PER_SECOND` (default 20)
- For Boktai 1, mGBA uses 10 internal steps despite 8 visible bars — the firmware compensates (disable via `HID_BOKTAI1_MGBA_10_STEP_WORKAROUND = false` if fixed)
- **Button remapping:** Change `HID_BUTTON_DEC` and `HID_BUTTON_INC` in config.h to use different buttons (see `XboxGamepadDevice.h` for available constants)

**Single Analog Mode specifics:**
- Works over both Bluetooth and USB XInput.
- Updates immediately on bar change (no rate throttling)
- Maps bar count to a proportional deflection on a configurable analog axis (default: Right X+)
- Optionally holds a configurable "meter unlock" button (default: R3) while the axis is active (`HID_METER_UNLOCK_BUTTON_ENABLED`). Single Analog mode uses midpoint band mapping, so even 0 bars sends a small deflection (not exact center).
- Bluetooth path: the unlock button is released/re-pressed on each deflection change and periodically (`HID_METER_UNLOCK_REFRESH_MS`, default 1000ms) so apps/emulators that start listening late can still register it as held.
- USB path: the unlock button is held while active and updated with normal USB reports (no periodic release/re-press refresh cycle).
- Axis and unlock button are configurable in config.h (`HID_SINGLE_ANALOG_AXIS`, `HID_METER_UNLOCK_BUTTON`)

### Bluetooth-Specific Details

The device advertises as "Ojo del Sol Sensor" (configurable via `BLE_DEVICE_NAME`).

**Pairing/connection behavior:**
- Times out after `BLE_PAIRING_TIMEOUT_MS` (default 60s)
- Re-enters pairing if connection drops
- **Pairing restarts on wake** — after waking from sleep, the device re-advertises for pairing
- Bluetooth icon flashes while pairing, solid when connected

### Screensaver

After `SCREENSAVER_TIME` minutes (default 3) of no button activity, the display shows a bouncing "Ojo del Sol" logo. Press any button to wake. Set `SCREENSAVER_TIME = 0` to disable (but this affects OLED lifespan). Alternatively, set `SCREENSAVER_ACTIVE = false` to disable it without changing the time value. The status row appears only when battery data is available and/or BLE is actively connected or pairing.

----------------------------------------------------------------------

## Calibration

### Automatic Mode (Default)

Bar thresholds are calculated from two values in config.h:

```c
const bool AUTO_MODE = true;
const float AUTO_UV_MIN = 1.000;   // UV Index for 1 bar
const float AUTO_UV_SATURATION = 13.000;   // UV Index ceiling (output clamp)
```

Bars are distributed non-linearly following the original Boktai cartridge's thresholds (per [Raphi's sensor graph](https://raphi.xyz/~raphi/boktai/sensor_graph/)), scaled across the configured UV range. `AUTO_UV_MIN` is where bar 1 starts; `AUTO_UV_SATURATION` is where the highest bar starts (and all values above are clamped to max).

**Original cartridge bar thresholds** (0–140 scale; source: [raphi.xyz/~raphi/boktai/sensor_graph/](https://raphi.xyz/~raphi/boktai/sensor_graph/)):

```
Exclusive upper bound
Bars    Boktai 1    Boktai 2 & 3
0       1           1
1       7           6
2       16          13
3       28          23
4       44          35
5       67          50
6       98          67
7       140         87
8       (max)       110
9       —           140
10      —           (max)
```

`AUTO_UV_MIN` maps to scaled value 1 and `AUTO_UV_SATURATION` maps to 140.

### Manual Mode

Set `AUTO_MODE = false` to use per-game `BOKTAI_1/2/3_UV_MIN` and `BOKTAI_1/2/3_UV_SATURATION` values (`BOKTAI_1_UV_MIN`, `BOKTAI_1_UV_SATURATION`, etc.). Bar thresholds follow the same non-linear cartridge curve as AUTO mode, but each game can use a different UV range.

### Transparent Enclosure Compensation

If the sensor is behind a transparent window, model that window as transmission loss:

```c
const bool UV_ENCLOSURE_COMP_ENABLED = true;
const float UV_ENCLOSURE_TRANSMITTANCE = 0.42; // 42% UV passes through the window
const float UV_ENCLOSURE_UVI_OFFSET = 0.000;     // Optional bias correction
```

The current repo default in `config.h` is `0.833`, based on the enclosure linked above; expect to recalibrate this value for your own case/window material.

Firmware applies compensation once after raw-to-UVI conversion:

`corrected_uvi = max(0, (measured_uvi - offset) / transmittance)`

Quick calibration:
1. Temporarily set `UV_ENCLOSURE_COMP_ENABLED = false` while collecting calibration readings.
2. Take side-by-side readings at the same time/angle: one sensor open-air, one in-box.
3. Compute `in_box_uvi / open_air_uvi` across several samples.
4. Use the median ratio as `UV_ENCLOSURE_TRANSMITTANCE`.
5. Re-enable compensation (`UV_ENCLOSURE_COMP_ENABLED = true`).

**Threshold calibration mode:** By default (`UV_THRESHOLDS_CALIBRATED_OPEN_AIR = true`), all threshold values in config.h (`AUTO_UV_MIN`, `AUTO_UV_SATURATION`, and the per-game `BOKTAI_*_UV_MIN`/`BOKTAI_*_UV_SATURATION` values) are assumed to be open-air UVI values — set while the case is open or without an enclosure. The firmware compensates sensor readings to open-air equivalent before comparing against them. Set `UV_THRESHOLDS_CALIBRATED_OPEN_AIR = false` if you instead calibrated thresholds with the sensor inside the closed enclosure; bar comparison will then use the raw (uncompensated) reading directly.

For open-air builds (sensor always exposed when device is in use): set `UV_ENCLOSURE_COMP_ENABLED = false` and leave `UV_THRESHOLDS_CALIBRATED_OPEN_AIR = true`. Compensation is a no-op so raw equals compensated.

Note: On the XInput screen, `UVI` is the measured (pre-compensation) value and `UVI comp'ed` is the value used for bar calculation (post-compensation; post-smoothing if `UVI_SMOOTHING_ENABLED = true`).

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
- Wire the LTR390 directly to `3V3`, same as the OLED
- Cutting the LTR390 breakout LED trace is recommended because the LED is hardwired to VIN
- Battery sensing and low-voltage cutoff are disabled by default; enable after wiring and validating the battery divider (`BATTERY_SENSE_ENABLED`, `BATTERY_CUTOFF_ENABLED`)
- Battery sampling/cutoff checks run on a timed loop and are not dependent on UV "new data" events
- OLED uses Display OFF + Charge Pump OFF commands
- On wake, firmware explicitly re-enables the OLED charge pump/display before drawing
- Deep sleep current: ~10µA (varies with module pull-ups)

### UV Blocking Warning
Most glass and many plastics block UV strongly (often 90%+). Compensation can correct scale loss, but it cannot recover signal if too little UV reaches the sensor. Prefer an open aperture, quartz glass, or UV-transparent acrylic.

----------------------------------------------------------------------

## Troubleshooting

### UV sensor reads 0 or very low
If button presses are being seen, Serial should print `Device button pressed` whenever the device button is pressed.
1. Set `DEBUG_SERIAL = true` and check Serial Monitor (115200 baud). Serial output is only active in CDC mode — enter CDC mode from the XInput screen (hold 2s), or set `USB_HID_ENABLED = false` to always use CDC.
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
1. Hold button for full 2 seconds
2. A short press should immediately show the "Hold 2s to power on" prompt
3. After 10s of no activity, it returns to sleep
4. If the OLED fails to initialize, the device enters deep sleep after about 2 seconds using the normal sleep path (same cleanup and button-release debounce as manual sleep). Check I2C wiring and `DISPLAY_I2C_ADDR` in config.h (some modules use `0x3D` instead of `0x3C`)

### Display is blank, mirrored, or inverted after entering CDC mode
1. Update to the latest firmware (the boot sequence now re-asserts OLED power and canonical panel state after the software restart)
2. Confirm `Tools > Upload Mode` is set to `USB-OTG CDC (TinyUSB)`
3. If needed, re-enter CDC mode once (hold 2s on the XInput screen), then upload again

### BLE crashes at startup (`block_locate_free` TLSF assert)
This was a known compatibility issue between esp32 core `3.3.7` and older versions of NimBLE-Arduino. It is resolved as of NimBLE-Arduino v2.3.9 — update the library via Arduino Library Manager and rebuild.

----------------------------------------------------------------------

## Future Plans

- [ ] Lunar Knights support?

----------------------------------------------------------------------

## Credits & Links

- **[Prof9](https://github.com/Prof9/Boktai-Solar-Sensor-Patches)** — GBA link patches, IPS patches, and link protocol are based on Prof9's work (MIT License — see [Prof9's license.txt](Prof9's%20license.txt)) Prof9 also provided a ridiculous amount of help, patience, and kindness. Many thanks!
- **[Raphi](https://raphi.xyz/~raphi/boktai/sensor_graph/)** — Original Boktai cartridge bar threshold tables, reverse-engineered from the original hardware. This saved me a lot of work!
- **[Mystfit](https://github.com/Mystfit/ESP32-BLE-CompositeHID)** — ESP32-BLE-CompositeHID library (included as `ESP32-BLE-CompositeHID-master.zip`)
- **[Kalasoiro](https://github.com/Kalasoiro/esp32s3-tinyusb-xinput)** - XInput TinyUSB reference implementation (included as `esp32s3-tinyusb-xinput-main.zip`)
- **LanHikariDS** — Additional support
- [GBATEK Solar Sensor Documentation](https://problemkaputt.de/gbatek.htm)
- [Seeed XIAO ESP32S3 Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [Adafruit LTR390](https://www.adafruit.com/product/4831)

----------------------------------------------------------------------

## Legal Disclaimer

This project is for hobbyist and entertainment use only and does not provide medical advice. Consult a physician or other qualified healthcare professional for guidance on safe and appropriate sunlight exposure for your situation.

Do not rely on video games or the Ojo del Sol as an indicator of how much sunlight or UV exposure you should get. They are gameplay devices, not health or safety instruments.

This project is an independent fan-made hardware/firmware project and is not affiliated with, endorsed by, or sponsored by Konami or Hideo Kojima.
