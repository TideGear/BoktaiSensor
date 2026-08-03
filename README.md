I've done this all at my own expense.\
Please consider donating via...

<a href="https://ko-fi.com/TideGear"><img src="https://storage.ko-fi.com/cdn/kofi6.png" height="24"></a> or 
<a href="https://www.paypal.com/donate/?hosted_button_id=THMF458QTBCAL"><img src="https://i.postimg.cc/VvChGNyd/Pay-Pal.png" height="24"></a>

https://ko-fi.com/TideGear

https://www.paypal.com/donate/?hosted_button_id=THMF458QTBCAL

----------------------------------------------------------------------

**Important!: This project is all very new, experimental, lightly-tested, and subject to change.\
I'm looking for serious contributors to...**

**1. Help tune the default device settings to *exactly* match how all three games handle a range of sunlight.** The defaults are roughly calibrated against real sunlight and a real Boktai 3 cartridge, but more testing across conditions and games is welcome. Where testing is ambiguous (e.g. flickering meter bar in the game), I tend to skew toward whatever is more "challenging" to achieve (seeking out more sunlight). The Ojo del Sol is meant to be the "no cheating" solution. However, keep in mind, I am talking about nit-picky, fractions of UVI.\
**2. Add support via Single Analog Control Mode to their emulators.**\
**3. Test and fix the code if needed.**\
**4. Create a cool 3D-printable case.**\
**5. You tell me!**

**...so please contact me if you are interested at TideGear @at@ gmail .dot. com !**

**Also contact me if you want to buy a pre-assembled device or kit!**

Photos of a build: https://photos.app.goo.gl/MvNCn2qLSqU6MgfA8 \
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

## ⚡ Branch: LilyGO Mini E-Paper S3 only

> This branch (`lilygo-mini-epaper-s3`) supports **one board: the
> [LilyGO Mini E-Paper S3](https://lilygo.cc/en-us/products/mini-e-paper-core)**
> (ESP32-S3, built-in 1.02" GDEW0102T4 e-paper panel). The Seeed XIAO
> ESP32S3 and LilyGO T-QT Pro builds — and the board-selection switch in
> `config.h` — are on `main`.
>
> **Also removed on this branch, both because e-paper makes them pointless:**
> - **UV enclosure compensation** — this build runs the LTR390 exposed (no
>   window over the sensor), so there is no transmission loss to model.
> - **The screensaver** — it exists on `main` to limit OLED burn-in, which
>   e-paper does not have.
>
> Both still live on `main` if you need them.
>
> **Status: untested on real hardware.** The firmware compiles clean and the
> pin map follows LilyGO's own board header, but nothing here has been run on
> an actual unit. Two vendor documentation conflicts to be aware of before you
> start troubleshooting — both are one-line changes in `config.h`:
>
> | Setting | This branch uses | Product-page pinout image says | Why |
> |---------|------------------|-------------------------------|-----|
> | `EPD_POWER_PIN` (panel rail enable) | `42` | `21` | LilyGO's `boards.h` (what their shipped examples use) says 42, and labels 21 as an unrelated `MOTOR` pin that the header breaks out |
> | `BAT_PIN` (battery ADC) | `2` | `43` | GPIO43 is U0TXD and has **no ADC channel** on the ESP32-S3 (ADC1 = GPIO1–10, ADC2 = GPIO11–20), so it cannot be read with `analogRead()` |
>
> If the panel stays blank with no other symptom, try `EPD_POWER_PIN = 21`
> first. Please report results either way via a GitHub issue.

----------------------------------------------------------------------

## Quick Start

**Recommended setup:** Incremental Mode (`HID_CONTROL_MODE = 0`) with mGBA libretro core (RetroArch), over either Bluetooth or USB XInput

1. Build the hardware (see [Hardware](#hardware) below)
2. Flash the firmware (see [Firmware Setup](#firmware-setup))
3. For mGBA libretro, set `HID_CONTROL_MODE = 0` in `config.h`
4. Pair the device as a Bluetooth controller in your OS
5. In mGBA (libretro), map **L3** to solar sensor decrease and **R3** to increase
6. Power on: hold button 2 seconds
7. Tap the switch left/right to select game (BOKTAI 1, 2, or 3). (Optional) Tap to the **XInput** screen to view UV raw, UVI, smoothed UVI, ADC avg, and battery voltage.
8. Point sensor toward the sky — the device syncs the in-game meter automatically
9. Hold the center press 2 seconds on a game screen to sleep. On the **XInput** screen, hold 2 seconds to enter CDC mode (USB serial / firmware upload).

**Important:** Only wake the device once you are in-game. The controller inputs it sends (button presses, stick deflections) may cause unwanted behavior in menus or other apps. When possible, put the device to sleep (hold 2s) when you are not actively playing a Boktai game.

**Important:** This branch expects the LTR390 to be **exposed** — mounted through an open aperture, not behind a case window. Standard glass and most plastics block 90%+ of UV, and the compensation pass that models a window as transmission loss is not present here (see the branch banner above). Keep the sensor uncovered.

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

In CDC mode the firmware runs fully (UV sensing, game display, BLE, GBA link). The screen header shows **CDC** instead of **XInput**. Hold the button for 2 seconds from any screen to exit CDC mode and sleep; on next wake, the device boots back into XInput mode.

As an alternative, set `USB_HID_ENABLED = false` to have the firmware boot directly into CDC on wake.

### 3. GBA Link Cable (Best Option for Flash Carts)

This outputs a framed 3-wire value (SC + SD + SO) for use with a physical GBA via link cable. On the Mini E-Paper S3 it uses the IO16/IO17/IO18 header pads.

**Requirements:**
- Use the updated ROM hack IPS patches in this repo's `GBA Link Patches/` folder (`Source/` contains the ASM sources). Many thanks to Prof9 for the original proof-of-concept code!
- Recommended: install a real GBA link port on the Ojo del Sol and wire it as shown in [GBA Link Port Wiring.png](GBA%20Link%20Port%20Wiring.png). Link ports can be ordered here: https://a.co/d/00r3xzuj
- A breakout board also works, but the link cable can come loose easily if you move around much, so the real link port is the more robust option.
- Connect the Ojo del Sol to the **Player 1 (P1)** side of the cable and the GBA to the **Player 2 (P2)** side.

**Note:** The English translation of Boktai 3 should work with the patch, but this is currently untested.

**Limitation:** Boktai's normal link-cable modes (e.g., multiplayer) are **not compatible** with this and may never be. To use multiplayer alongside the Ojo del Sol, you'll need an emulator with link support (I've contacted Pizza Boy A Pro and Linkboy devs — TBD).

### 4. Display Only / Manual Entry (Not Recommended)
Read the bar count from the screen and enter it manually using Prof9's ROM hacks (https://github.com/Prof9/Boktai-Solar-Sensor-Patches). If you intend to use manual entry, **do not** use the ROM hacks in the GBA Link Patches directory. Those are for the GBA link cable method above.

**Note:** E-paper is the one display in this project that is genuinely readable in direct sunlight, so manual entry is more viable on this board than on the OLED/TFT builds. The bar count refreshes as soon as it changes, so the screen is not meaningfully behind the other outputs.

----------------------------------------------------------------------

## Hardware

The [LilyGO Mini E-Paper S3](https://lilygo.cc/en-us/products/mini-e-paper-core) has a built-in 1.02" 80x128 GDEW0102T4 e-paper panel, a 3-way tactile switch, a plastic shell, battery charging, and battery voltage sensing — so the only wiring needed is the LTR390 and a battery.

**Components:**
- LilyGO Mini E-Paper S3 (ESP32-S3-FN4R2 — 4MB flash, 2MB PSRAM)
- Adafruit LTR390 UV Sensor (I2C) — soldered to the IO7/IO8 header pads (the board has no Qwiic/STEMMA connector)
- 3.7V LiPo battery (the shell has a battery bay; use the board's battery connector)

**Why e-paper suits this project:** the screen is *more* readable in direct sunlight, not less — which is exactly where this device gets used. It also holds its image with zero power, so the panel is put to sleep whenever the content is not changing.

**Board notes:**
- The UI renders as a 128x64 region centered on the 128x80 landscape panel (8 blank rows top and bottom). Set `EPAPER_DISPLAY_ROTATION` in `config.h` to `1` or `3` (the two landscape orientations, 180° apart) to match your case orientation. Portrait rotations (`0`/`2`) are not usable — the 128-pixel-wide UI does not fit across the panel's short axis.
- Battery sensing is built in (2:1 divider, `BAT_PIN = 2`) and enabled by default; no resistors needed.
- GBA link output uses the IO16 (SC), IO17 (SD), IO18 (SO) header pads plus a GND pad. See [GBA Link Cable Wiring](#gba-link-cable-wiring-optional).
- The microSD slot (IO38–IO41) is unused by this firmware. Avoid those pins if you repurpose headers.
- **Refresh behavior:** e-paper refreshes take hundreds of milliseconds and the panel wears with each one, so the firmware does not redraw as freely as an OLED. Frames identical to what is already on screen are skipped, and every `EPAPER_FULL_REFRESH_EVERY`th update (default 30) is promoted to a full refresh to clear accumulated ghosting. Updates run in **two lanes**:
  - **Priority** — a changed bar count, or a screen change you just pressed a button for. Refreshes immediately (`EPAPER_PRIORITY_MIN_REFRESH_MS`, default `0` = no floor), so the sun gauge tracks the sensor as closely as the panel allows.
  - **Routine** — incidental churn like the UVI decimals ticking over or the battery percentage moving. Held to `EPAPER_MIN_REFRESH_MS` (default 1500ms).

  Because the gauge no longer waits on the routine lane, **raising `EPAPER_MIN_REFRESH_MS` is a cheap way to cut panel wear** — the only cost is a staler UVI readout on the game screens. The HID and GBA link outputs were never rate-limited and still update as fast as the sensor produces data.

### Wiring

Only the sensor and battery need wiring. Everything else is on-board.

| Component | Mini E-Paper S3 pad | Notes |
|-----------|---------------------|-------|
| LTR390 VIN | 3v3 | 3.3V power |
| LTR390 GND | GND | |
| LTR390 SDA | IO7 | I2C data (configurable via `I2C_SDA_PIN`) |
| LTR390 SCL | IO8 | I2C clock (configurable via `I2C_SCL_PIN`) |
| Battery | on-board battery connector | Shell has a bay for the pack |

I2C needs pull-ups on SDA/SCL; the Adafruit LTR390 breakout includes 10K pull-ups on board, so no extra parts are needed with it.

**Built-in pins used by the board itself** (do not repurpose): IO10–IO15 and IO42 (e-paper panel), IO0/IO3/IO4 (3-way switch), IO2 (battery ADC), IO38–IO41 (microSD).

### Battery Monitoring

Built in — a 2:1 divider on `BAT_PIN = 2`, enabled by default (`BATTERY_SENSE_ENABLED = true`). No external resistors needed.

**Calibration:** If battery % is wrong at full charge, adjust `VOLT_DIVIDER_MULT` in config.h (increase if reading low, decrease if high). If the reading is implausible rather than merely off, see the `BAT_PIN` note in the branch banner above.

### GBA Link Cable Wiring (Optional)

Only needed if using GBA Link output mode. Recommended: install a real GBA link port on the Ojo del Sol and wire it as shown in [GBA Link Port Wiring.png](GBA%20Link%20Port%20Wiring.png). Link ports can be ordered here: https://a.co/d/00r3xzuj

A breakout board such as the [Palmr breakout board](https://github.com/Palmr/gb-link-cable) still works, but the link cable can come loose easily if you intend to move around much. An actual link port is a much more robust solution.

**Recommended Ojo-side link port wiring (Mini E-Paper S3),** using the header pads:

| Ojo Connection | Ojo-side GBA Port Pin Type | Equivalent P1 Breakout Pad | Notes |
|----------------|----------------------------|----------------------------|-------|
| IO18 pad | SI | SO | Payload bit 0 |
| GND pad | GND | SI | Shared ground |
| IO17 pad | SD | SD | Payload bit 1 |
| IO16 pad | SC | SC | Frame phase; toggles every `GBA_LINK_FRAME_TOGGLE_MS` |

Other free header pads (IO5, IO6, IO9, IO21) also work if these are inconvenient; set them via `GBA_PIN_SC`/`GBA_PIN_SD`/`GBA_PIN_SO` in `config.h`. Avoid IO7/IO8 (I2C) and IO38–IO41 (microSD).

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
- Keep all signals at 3.3V logic (the ESP32-S3 is 3.3V native, so no level shifting is needed)
- The ESP32 writes data pins (SD, SO) before the phase pin (SC) so the GBA always samples stable data when it detects a phase edge. The GBA-side ASM patches include a consecutive-match check that discards any single-frame misread, adding one frame (~17ms) of latency on real bar changes but eliminating visible glitches.


----------------------------------------------------------------------

## Firmware Setup

### Libraries Required

**Install via Arduino Library Manager:**
- Adafruit LTR390 Library
- Adafruit GFX Library
- Adafruit BusIO
- NimBLE-Arduino (by h2zero)
- Callback (by Tom Stewart)
- GxEPD2 (by Jean-Marc Zingg) — the e-paper driver; no configuration needed, the panel pins are fixed board wiring set in `config.h`

**Install manually (included in repo as ESP32-BLE-CompositeHID-master.zip, or download from GitHub):**
- ESP32-BLE-CompositeHID: https://github.com/Mystfit/ESP32-BLE-CompositeHID

**Why GxEPD2 and not the bundled GxEPD from LilyGO's repo?** LilyGO's [Mini-Epaper repo](https://github.com/Xinyuan-LilyGO/LilyGO-Mini-Epaper) vendors the older GxEPD library and targets esp32 core 2.0.x. GxEPD2 is its actively maintained successor, has a native GDEW0102T4/UC8175 driver with fast partial update, works on core 3.x (which this project requires), and installs straight from the Library Manager with no vendored copy to keep in sync.

### Board Settings (Arduino IDE)

- **Boards Manager URL:** `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- **Board package:** esp32 by Espressif Systems
- **Recommended version:** `3.3.7` or newer (verified building on `3.3.10`)
- **Board:** **ESP32S3 Dev Module** (no Mini E-Paper entry exists in the ESP32 core) with these settings — everything not listed stays at its default:
  - **Flash Size:** 4MB (32Mb) — the S3 variant is `ESP32-S3FN4R2`
  - **PSRAM:** QSPI PSRAM
  - **Partition Scheme:** Huge APP (3MB No OTA/1MB SPIFFS) — the sketch is too large for the default 1.2MB app partition
  - **Flash Mode:** QIO 80MHz, **CPU Frequency:** 240MHz, **Events/Arduino Run On:** Core 1
- **USB Mode:** USB-OTG (TinyUSB)
- **USB CDC On Boot:** Enabled (required for CDC mode; normal runtime remains XInput unless you enter CDC mode from the XInput screen or set `USB_HID_ENABLED = false`)
- **Upload Mode:** USB-OTG CDC (TinyUSB) (required for CDC-mode uploads; UART0/Hardware CDC will fail to auto-connect in this setup)
- If CDC UI appears inconsistent/garbled after failed uploads, do one upload with **Erase All Flash Before Sketch Upload = Enabled**, then set it back to Disabled.
- **To force download mode** (if an upload will not start): hold the switch's center press (GPIO0 — it is also the boot button) while plugging in USB.

For reference, the exact command this branch was verified building with:

```
arduino-cli compile \
  -b esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=4M,PSRAM=enabled,PartitionScheme=huge_app \
  BoktaiSensor.ino
```

----------------------------------------------------------------------

## Usage

### Button Controls

The board has a single 3-way tactile switch on the top edge: **left (GPIO3)**, **center press (GPIO0)**, **right (GPIO4)**.

| Direction | Tap | Hold 2s |
|-----------|-----|---------|
| **Center press** (GPIO0) | Next screen | Power on/off, CDC mode — see below |
| **Right** (GPIO4) | Next screen | — |
| **Left** (GPIO3) | Previous screen | — |

Tap directions are configurable via `BUTTON_TAP_STEP` / `BUTTON2_TAP_STEP` / `BUTTON3_TAP_STEP` in config.h.

**When device is ON:**
- **Tap:** Cycle screens (BOKTAI 1 → 2 → 3 → XInput → 1...). The XInput screen is always present and doubles as the diagnostics screen.
- **Hold center 2s on a game screen:** Power OFF (deep sleep, ~10uA)
- **Hold center 2s on the XInput screen:** Restart into CDC mode for USB serial output and firmware upload
- **Hold center 2s while in CDC mode:** Exit CDC mode and sleep (next wake returns to XInput mode)

**When waking from sleep:**
- **Hold center 2s:** Power ON
- **Short press/tap on center:** Immediately shows "Hold 2s to power on"
- If there is no button activity for 10 seconds, it returns to sleep

Only the **center press** can wake the device from deep sleep, so it is the power button. Left and right do nothing while asleep — this is why they have no long-press action.

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

### No Screensaver

**There is no screensaver on this branch.** The OLED builds on `main` use one to limit burn-in; e-paper has none to limit, so it was removed rather than reworked. The screen simply shows the current reading until you change screens or sleep the device.

One consequence worth knowing: the screensaver was also the panel's idle low-power state (it hibernated the controller after 3 minutes). Without it the panel stays powered for as long as the device is on, so idle current while ON is somewhat higher. Sleep behavior is unchanged — hold the center press 2s and the panel is hibernated and its rail cut.

### Running Without the Sensor

The LTR390 is optional at boot. If it does not answer on I2C, the device shows

```
LTR390 not found
Demo: bar sweep
```

for about 2.5 seconds and then **continues booting normally**. Everything that does not depend on UV keeps working: display, buttons, screen cycling, battery monitoring and cutoff, Bluetooth/USB HID pairing and reports, GBA link output, and CDC mode.

With no sensor:
- The sun gauge **sweeps 0 → max → 0 continuously**, as fast as the panel can redraw (`NO_SENSOR_BAR_SWEEP`, default on)
- UV readouts show **`N/A`** rather than `0`, on both the game screens and the XInput screen — a missing sensor should not be mistakable for darkness
- Every sensor access is skipped, so a floating I2C bus cannot produce phantom readings

The sweep is paced by *completed panel refreshes*, one bar per refresh, which makes it two useful things at once:

1. **A panel benchmark.** The visible sweep rate *is* the panel's real refresh rate. Time a full 0→max→0 cycle and divide by `2 × bars` to get milliseconds per refresh — handy for deciding whether `EPAPER_MIN_REFRESH_MS` and the priority lane are tuned sensibly for your unit.
2. **An end-to-end output test.** The swept value is sent over Bluetooth/USB HID and the GBA link like any real reading, so you can watch an emulator's solar meter swing, or scope the link pins, with no sensor attached.

Pacing to refreshes rather than loop iterations matters: the loop runs at roughly 1kHz, and stepping a bar that often would flood the BLE/USB stacks with thousands of reports per second for frames the panel never displayed.

Set `NO_SENSOR_BAR_SWEEP = false` to leave the gauge at 0 instead — do that if you are testing HID pairing or emulator button mapping and a swinging meter would get in the way. Note that the sweep refreshes the panel continuously for as long as the device is on without a sensor, which is real panel wear; it is a bring-up and demo aid, not something to leave running for hours.

This makes the board usable for bring-up and for testing the display, buttons, HID pairing, and link wiring before the sensor is attached. The sensor is only probed at boot, so **restart after connecting it.**

----------------------------------------------------------------------

## Calibration

### Automatic Mode (Default)

Bar thresholds are calculated from two values in config.h:

```c
const bool AUTO_MODE = true;
const float AUTO_UV_MIN = 1.000;   // UV Index for 1 bar
const float AUTO_UV_SATURATION = 13.100;   // UV Index ceiling (output clamp)
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

### Enclosure Compensation — not on this branch

This build runs the LTR390 exposed, so all threshold values in config.h (`AUTO_UV_MIN`, `AUTO_UV_SATURATION`, and the per-game `BOKTAI_*_UV_MIN`/`BOKTAI_*_UV_SATURATION` values) are plain open-air UVI readings and are compared against the sensor reading directly. There is no window-transmission correction step.

If you do put the sensor behind a window, the compensation code is on `main` (`UV_ENCLOSURE_COMP_ENABLED`, `UV_ENCLOSURE_TRANSMITTANCE`, `UV_ENCLOSURE_UVI_OFFSET`, `UV_THRESHOLDS_CALIBRATED_OPEN_AIR`). See also the [UV Blocking Warning](#uv-blocking-warning) below — compensation corrects scale loss, it cannot recover signal that never reached the sensor.

Note: On the XInput screen, `UVI` is the value straight from the sensor and `UVI smoothed` is the value used for bar calculation. They are identical unless `UVI_SMOOTHING_ENABLED = true`.

### Stability Tuning

If the bar display feels jumpy:
- `UVI_SMOOTHING_ENABLED`: Applies exponential smoothing (default: `false`; set `true` to smooth)
- `UVI_SMOOTHING_ALPHA`: Controls how much weight new readings get vs. the running average (default: `0.5`; higher = faster response, lower = smoother). Setting `UVI_SMOOTHING_ALPHA = 1.0` is the same as `UVI_SMOOTHING_ENABLED = false`.
- `BAR_HYSTERESIS_ENABLED`: Enables bar hysteresis (default: `false`). Setting `BAR_HYSTERESIS_ENABLED = false` is the same as `BAR_HYSTERESIS = 0.0`.
- `BAR_HYSTERESIS`: Requires UV margin before changing bars (default: `0.200`; only used when `BAR_HYSTERESIS_ENABLED = true`)

----------------------------------------------------------------------

## Technical Notes

### Original Boktai Cartridge Sensor (from GBATEK and Drakodan)
- **Chip**: Hamamatsu Photonics G5842 (UV photodiode with 8-bit ADC)
- **Spectral range**: 260–400nm, centered at ~370nm (UV-A dominant)
- Values **inverted**: 0xE8 = darkness, 0x50 = max gauge, 0x00 = extreme
- Accessed via GPIO at 0x80000C4-0x80000C8

### LTR390 UV Sensor
- Reference sensitivity: 2300 counts/UVI at 18x gain, 400ms integration
- Formula: `UVI = raw / 2300` (at our settings: gain 18x, 20-bit/400ms, 500ms rate). The sensor is exposed on this branch, so no enclosure correction is applied on top.
- Peak response: 300-350nm (UV-A/UV-B)
- **Not inverted**: higher values = more UV

### E-Paper Panel (GDEW0102T4 / UC8175)
- 80x128 native (portrait), driven at rotation 1 or 3 for a 128x80 landscape UI
- The UI is authored at 128x64 monochrome and rendered into an offscreen 1-bit canvas (`EPaperDisplay.h`, a `GFXcanvas1` subclass mirroring the `Adafruit_SSD1306` API the sketch uses), then pushed centered onto the panel. Set canvas pixels become **black ink** — the inverse of the OLED builds — so the artwork reads correctly on paper.
- Pushes are skipped when the new frame is pixel-identical to what is already on the panel, so repeated identical draws (e.g. the "Hold 2s to power on" prompt redrawing while the button is held) cost nothing
- Partial (differential) refresh is used by default, with a full refresh every `EPAPER_FULL_REFRESH_EVERY` pushes to clear ghosting. A full refresh is also forced after any panel hibernate or power cut, because the controller loses the previous-image RAM that a differential refresh diffs against.

### Power Management
- Wire the LTR390 directly to `3v3`
- Cutting the LTR390 breakout LED trace is recommended because the LED is hardwired to VIN
- Battery sensing and low-voltage cutoff are both enabled by default on this board (the divider is built in). Validate the reading on the XInput screen before trusting the cutoff.
- Battery sampling/cutoff checks run on a timed loop and are not dependent on UV "new data" events
- **No idle panel hibernation while ON.** Removing the screensaver removed the only path that hibernated the controller mid-session, so the panel rail stays up until you sleep the device. Adding an idle hibernate that keeps the last image on screen (no logo, no visual change) would recover this if idle runtime matters.
- **Before deep sleep** the firmware clears the canvas and pushes that blank frame *before* cutting the panel rail. E-paper retains its last image with no power, so skipping this would leave the UI visible for the whole sleep.
- The panel rail enable pin is latched off through deep sleep (`gpio_hold_en` + `gpio_deep_sleep_hold_en`) and released on wake
- Deep sleep current: ~10µA (varies with module pull-ups)

### UV Blocking Warning
Most glass and many plastics block UV strongly (often 90%+). Prefer an open aperture, quartz glass, or UV-transparent acrylic. This branch has no enclosure compensation at all, so anything over the sensor directly scales your readings down.

----------------------------------------------------------------------

## Troubleshooting

### "LTR390 not found" at boot, and the gauge sweeps up and down on its own
The sensor did not answer on I2C. This is **not fatal** — the device finishes booting and runs normally without it, and the sweeping gauge is the deliberate no-sensor demo (see [Running Without the Sensor](#running-without-the-sensor)). UV rows read `N/A` instead of `0`. Check the wiring to the IO7/IO8 pads, that the breakout has pull-ups, and that it is powered from `3v3`. Reconnecting the sensor requires a restart.

If you want a sensorless device to sit still instead of sweeping, set `NO_SENSOR_BAR_SWEEP = false`.

### UV sensor reads 0 or very low
First confirm you did **not** see "LTR390 not found" at boot — if the readouts say `N/A` rather than a number, the sensor was never detected (above).

If button presses are being seen, Serial should print `Device button pressed` whenever the device button is pressed.
1. Set `DEBUG_SERIAL = true` and check Serial Monitor (115200 baud). Serial output is only active in CDC mode — enter CDC mode from the XInput screen (hold 2s), or set `USB_HID_ENABLED = false` to always use CDC.
   - At UVI 6, expect ~13800 raw counts (6 × 2300)
   - If raw = 0, sensor may not be in UV mode
2. Ensure sensor faces the sky (not blocked by hand/case). This branch has no enclosure compensation — anything over the sensor scales readings down directly.
3. Check the LTR390 wiring to the IO7/IO8 pads and that the breakout's pull-ups are present
4. Indoor UV is near zero — test outside

### Battery shows 0% or an implausible voltage
Battery sensing is built in and enabled by default. If the ADC average on the XInput screen looks like noise or a stuck value rather than a plausible voltage, `BAT_PIN` may be wrong for your board revision — see the `BAT_PIN` row in the branch banner at the top.

### Battery % is wrong
Adjust `VOLT_DIVIDER_MULT` in config.h (increase if reading low, decrease if high).

### Device powers off quickly
Check `BATTERY_CUTOFF_ENABLED` and `VOLT_CUTOFF`. Both battery sensing and cutoff are on by default on this board; if the ADC reading is bad (above) the cutoff can fire spuriously — set `BATTERY_CUTOFF_ENABLED = false` while you sort it out.

### Device won't wake
1. Hold the switch's **center press** for a full 2 seconds. Left/right cannot wake the device — only GPIO0 is wake-capable.
2. A short center press should immediately show the "Hold 2s to power on" prompt
3. After 10s of no activity, it returns to sleep
4. If the display fails to initialize, the device enters deep sleep after about 2 seconds using the normal sleep path (same cleanup and button-release debounce as manual sleep)

### Panel is completely blank
1. First suspect `EPD_POWER_PIN` — try `21` instead of the default `42`. See the branch banner at the top for why this is ambiguous.
2. Set `USB_HID_ENABLED = false` temporarily and watch the serial monitor. The firmware prints a boot banner and a marker after each init stage (`Initializing display (GxEPD2 / GDEW0102T4)...` then `Display initialized`), so the last line tells you which step failed.
3. If `Display init failed` prints, `EPaperDisplay::begin()` rejected the panel — check the `EPD_*_PIN` values in config.h against your board.
4. Remember the device shows a blank panel *on purpose* while asleep. Confirm it is actually powered on (hold center 2s) before treating a white screen as a fault.

### Panel shows a stale screen, faint ghosting, or streaks
- Ghosting after many partial refreshes is normal for e-paper. Lower `EPAPER_FULL_REFRESH_EVERY` in config.h to clear it more often (at the cost of more visible full-refresh flashing).
- A stale image that never updates while the device is on usually means pushes are being suppressed. Set `DEBUG_SERIAL = true` and confirm UV data is arriving.
- The UVI decimals and battery percentage intentionally lag by up to `EPAPER_MIN_REFRESH_MS` (default 1500ms). The **bar count does not** — it refreshes immediately.

### The bar gauge refreshes too often / too much flicker
Bar changes bypass the routine refresh interval by design. If borderline light makes the count oscillate between two levels and the constant refreshing bothers you, enable `BAR_HYSTERESIS_ENABLED` in config.h — that stops the oscillation at the source rather than masking it. Raising `EPAPER_PRIORITY_MIN_REFRESH_MS` above `0` is the blunt alternative.

The bar count cannot change faster than the LTR390's 500ms measurement rate, and each refresh blocks until the panel's BUSY line clears, so there is a natural ceiling on how often this happens.

### The gauge still feels slow
At the default settings nothing in the firmware throttles bar updates — the remaining floor is the panel's own refresh time. If that is not fast enough on real hardware, the next lever is region-limited refreshes (updating only the changed rectangle instead of the whole panel). That is **not** implemented: the current UI spreads its changing elements (battery percentage at the top, bar digit and UVI in the middle, gauge at the bottom) across most of the panel, so a single bounding box would still cover roughly 50–75% of it. Worth revisiting with real timing numbers in hand.

Note that panel refreshes block the main loop, which briefly stalls GBA link phase toggling and HID reports. More frequent bar refreshes mean more frequent stalls — the GBA-side patch tolerates this (its consecutive-match check discards single-frame misreads), but it is the reason the routine lane stays throttled.

### Display looks wrong after entering CDC mode
1. Confirm `Tools > Upload Mode` is set to `USB-OTG CDC (TinyUSB)`
2. The boot sequence forces a full refresh after the software restart, which should clear any partial-refresh artifacts
3. If needed, re-enter CDC mode once (hold center 2s on the XInput screen), then upload again

### BLE crashes at startup (`block_locate_free` TLSF assert)
This was a known compatibility issue between esp32 core `3.3.7` and older versions of NimBLE-Arduino. It is resolved as of NimBLE-Arduino v2.3.9 — update the library via Arduino Library Manager and rebuild.

### Build fails with `GxEPD2_BW.h: No such file or directory`
Install "GxEPD2" (by Jean-Marc Zingg) via the Library Manager.

### Upload won't start / COM port not appearing
Hold the switch's center press (GPIO0, also the boot button) while plugging in USB to force download mode.

----------------------------------------------------------------------

## Future Plans

- [ ] Lunar Knights support?

----------------------------------------------------------------------

## Credits & Links

- **[Prof9](https://github.com/Prof9/Boktai-Solar-Sensor-Patches)** — GBA link patches, IPS patches, and link protocol are based on Prof9's work (MIT License — see [Prof9's license.txt](Prof9's%20license.txt)) Prof9 also provided a ridiculous amount of help, patience, and kindness. Many thanks!
- **[Raphi](https://raphi.xyz/~raphi/boktai/sensor_graph/)** — Original Boktai cartridge bar threshold tables, reverse-engineered from the original hardware. This saved me a lot of work!
- **[Mystfit](https://github.com/Mystfit/ESP32-BLE-CompositeHID)** — ESP32-BLE-CompositeHID library (included as `ESP32-BLE-CompositeHID-master.zip`)
- **[Kalasoiro](https://github.com/Kalasoiro/esp32s3-tinyusb-xinput)** - XInput TinyUSB reference implementation (included as `esp32s3-tinyusb-xinput-main.zip`)
- **LanHikariDS** — Additional support via Discord
- **[Drakodan](https://tasvideos.org/9369S)** — Original cartridge sensor chip identification (Hamamatsu Photonics G5842) and spectral range documentation
- **[Jean-Marc Zingg](https://github.com/ZinggJM/GxEPD2)** — GxEPD2, the e-paper display library this branch uses
- [GBATEK Solar Sensor Documentation](https://problemkaputt.de/gbatek.htm)
- [LilyGO Mini E-Paper S3 product page](https://lilygo.cc/en-us/products/mini-e-paper-core) and [LilyGO-Mini-Epaper repo](https://github.com/Xinyuan-LilyGO/LilyGO-Mini-Epaper) (source of the pin map)
- [Adafruit LTR390](https://www.adafruit.com/product/4831)

----------------------------------------------------------------------

## Legal Disclaimer

This project is for hobbyist and entertainment use only and does not provide medical advice. Consult a physician or other qualified healthcare professional for guidance on safe and appropriate sunlight exposure for your situation.

Do not rely on video games or the Ojo del Sol as an indicator of how much sunlight or UV exposure you should get. They are gameplay devices, not health or safety instruments.

This project is an independent fan-made hardware/firmware project and is not affiliated with, endorsed by, or sponsored by Konami or Hideo Kojima.
