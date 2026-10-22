# Ojo del Sol Calibrator — PARTLY TESTED

Part of [Ojo del Sol](../README.md). This device is only for collecting calibration data; the Ojo del Sol itself does not need it.

> **Status: partly tested. The cartridge side has not been tried yet.**
>
> **Seen working on an ESP32-S3-CAM (16 MB):**
> - It boots, and the LTR390 reads (near 0 indoors at night).
> - It logs sessions to flash, one row per second.
> - It joins your Wi-Fi and answers at `http://ojodelsol-calibrator.local`.
> - It sets its clock from internet time, correct to within a few seconds.
> - It serves the web page and the status and live APIs.
>
> **Not yet tried on the board:**
> - **The cartridge:** none was wired for that run, so every reading timed out, as expected. Bus wiring and timing are untested.
> - **Wi-Fi fallback:** the hotspot and its captive portal.
> - **Web app:** downloads, uploads and restore.
> - **Interfaces:** the serial commands.
> - **Endurance:** long runs.
>
> The sensor-reading method itself was checked against a Boktai 3 (J) ROM and against a GBxCart RW, but not on this board. Please report what works and what doesn't. See [Not verified yet](#not-verified-yet) for the full list.

A separate device that reads a **real Boktai cartridge's solar sensor** and an **LTR390** at the same moment, and saves both to the board's flash. Leave it in the sun for hours, then watch it live, browse past sessions and back up the logs from your phone over its own Wi-Fi hotspot (or over serial). This is the data needed to tune `AUTO_UV_MIN`, `AUTO_UV_SATURATION` and the bar thresholds against the real hardware.

**What it logs** (one CSV row per sample, once per second by default):

| Column | Meaning |
|--------|---------|
| `uptime_ms` | Milliseconds since power-up. The device has no clock of its own. On your Wi-Fi it gets the time from internet time servers; otherwise a phone opening the web app sends it. Each time it learns the time, the log gets a `# time_sync,<uptime_ms>,<epoch_ms>` comment line that maps uptime to wall-clock time (UTC) |
| `cart_status` | `ok`, or `timeout` if the cartridge's flag never rose (no cartridge, or a wiring fault) |
| `cart_raw` | The game's raw sensor value. About 230 in the dark, falling toward 0 as it gets brighter |
| `cart_bus` | `1` if the cartridge's GPIO registers read back what was written (a wiring check) |
| `cart_ms` | How long the cartridge read took |
| `ltr_status` | `ok`, `stale` (no fresh reading) or `absent` |
| `ltr_raw`, `uvi` | LTR390 UV counts and `counts / 2300` (same settings as the main firmware) |

## Hardware

Written for an **ESP32-S3-CAM board with the camera unused** (any ESP32-S3 works if you change the pins in [`config.h`](config.h)). The pins avoid PSRAM, the strapping pins, native USB and UART0.

| Signal | GPIO |
|--------|------|
| Cartridge /WR, /RD, /CS | 21, 47, 38 |
| Cartridge AD0, AD1, AD2, AD3 | 39, 40, 41, 42 |
| Cartridge AD5, AD6 | 2, 1 |
| LTR390 SDA, SCL | 4, 5 |

**Wire routing:** the whole cartridge bus is on the board's **right-hand header**. Hold the board with the USB-C ports at the bottom; the usable pins from the bottom up are GPIO **21, 47, 38, 39, 40, 41, 42, 2, 1**, which is exactly the cartridge pin order /WR, /RD, /CS, AD0, AD1, AD2, AD3, AD5, AD6. Run the nine wires in that order and none cross (the gaps between 47 and 38, and between 42 and 2, hold pins you can't use: GPIO48 is the on-board LED, 0/45 are strapping pins, 35–37 are PSRAM, 43/44 are the serial port). This direction also keeps the active-low /WR, /RD and /CS off GPIO1–14, which give a ~60 µs low pulse at power-up. The LTR390's SDA/SCL stay on the left header next to 3V3. GPIO39–42 are also the JTAG pins and GPIO38/39/40 are the SD card's CMD/CLK/DATA, so leave the SD slot empty. GPIO4/5 are the camera's control bus; the LTR390 (0x53) does not collide with the camera (0x30).

**Cartridge wiring** — the cartridge is 3.3 V like the ESP32, so no level shifter is needed. Only the lines needed to reach the GPIO registers at 0x080000C4–C8 are driven, so the rest are tied off:

| Cartridge pin(s) | Connect to |
|------------------|------------|
| VCC (pin 1) / GND (pin 32) | 3V3 / GND |
| /WR (3), /RD (4), /CS (5) | the three GPIOs above. A ~10 k pull-up to 3V3 on each is **optional**: it only keeps them idle while the ESP32 boots |
| AD0–AD3, AD5, AD6 (pins 6–9, 11, 12) | the six GPIOs above |
| AD4 (pin 10) and AD7–AD15 (pins 13–21) | each through ~10 k to GND (a bussed resistor network works too). These must be held low when the address is latched. A direct tie to GND should also work, because the cartridge drives zeros on these lines for the GPIO registers, but the resistors protect against the cartridge ever driving a 1 into ground |
| A16–A23 (pins 22–29) | GND |
| /CS2 (pin 30) | 3V3, wired directly (keeps the save RAM deselected; it is an input on the cartridge, so no resistor is needed) |
| CLK (pin 2), /IRQ (pin 31) | leave unconnected |

The pin numbers match the GBA cartridge edge connector pinout in [`References/gba_cartridge_pinout.svg`](../References/gba_cartridge_pinout.svg). The full wiring, including the resistors, is drawn in [`Ojo del Sol Calibrator Wiring.svg`](Ojo%20del%20Sol%20Calibrator%20Wiring.svg). Power the board off before inserting or removing the cartridge.

![Ojo del Sol Calibrator wiring](Ojo%20del%20Sol%20Calibrator%20Wiring.svg)

## Firmware setup

- Open `OjoDelSolCalibrator.ino` (the folder name must match the sketch). The sketch is several files in that folder (`WebApp.ino`, `WebPage.h`, `GbaCartSensor.h`, `Shared.h`, `config.h`, `partitions.csv`); Arduino builds them together, so keep them all together. It also uses [`GbaCartBus.h`](../GbaCartBus.h) from the parent folder: the cartridge bus code shared with the main Ojo del Sol firmware's cartridge sensor option. So build it from inside the full repo, not from a copy of this folder on its own.
- Libraries: **Adafruit LTR390 Library** (plus Adafruit BusIO). LittleFS, Preferences, WiFi, WebServer, DNSServer and ESPmDNS come with the esp32 core. There are no external web libraries and the web app needs no internet.
- Board: **ESP32S3 Dev Module**, with **Flash Size: 16MB**, **Partition Scheme: Custom** (this uses the `partitions.csv` in the sketch folder) and **USB CDC On Boot: Disabled** (the default, so Serial stays on UART0 / the TTL port). **PSRAM** can stay Disabled or be set to *OPI PSRAM* for boards that have it; the sketch does not use PSRAM either way, and the pins avoid GPIO 33–37, which octal PSRAM occupies. These are the same board settings as in the Metal-Gear-Mk-0 robot project (an ESP32-S3-CAM from the same maker, 16 MB flash, OPI PSRAM), where that setup is known to work. Both *Custom* and *Huge APP (3MB No OTA/1MB SPIFFS)* build here with this layout.
- **Partitions:** with the Wi-Fi web app the firmware is about 1 MB, which does **not** fit the usual 1 MB app slot (it filled 99% of it). The `partitions.csv` next to the sketch gives a **16 MB** module a 3 MB app slot and about 12.9 MB for logs; the build uses 33% of the app slot. **Check your board really has 16 MB**: the serial banner prints `Flash chip: 16 MB` at boot, and the web app shows a warning banner if it is smaller. **On an 8 MB module (for example the XIAO ESP32S3), do not use this file as it is**: delete or edit it (a 3 MB app slot leaves about 4.5 MB for logs) and pick a matching scheme.
- **First run:** the log partition has never been formatted, so the device starts with "storage not set up" and records nothing until you press **Format storage…** on the web app's Storage tab (or send `format` over serial). It does not format by itself, so a failed mount can never silently erase old logs.
- Log capacity: rows are 33–43 bytes, so one sample per second is roughly 120–155 KB per hour. 12.9 MB is therefore roughly **4 days** at the default 1-second interval (about 8 days at 2 s, or 3 weeks at 5 s); an 8 MB module with a 3 MB app slot would hold about a day and a half at 1 s. These are estimates from the row size, not measured on a device. All sessions on the device share the space, so download and delete earlier runs before a long one.
- Settings such as pins, sample interval, the Wi-Fi name/password and the cartridge tick time live in [`config.h`](config.h). Raise `SAMPLE_INTERVAL_MS` for longer runs. A sample takes about 1 second, so intervals shorter than that don't help.
- Serial is UART0 at 115200 baud, which is the board's **TTL** USB-C port.

## Using it

Power it up and it starts a new session file (`/cal_0001.csv`, `/cal_0002.csv`, …) on every boot. The numbering survives power cycles, and a session restored from a backup never gets its number reused. Data is committed to flash every 10 rows, so a sudden power cut loses at most the last 10. When less than 64 KB of flash is free it stops logging (`MIN_FREE_BYTES` in `config.h`); deleting sessions frees space and logging then resumes in a new session. The on-board LED is **green** when both sensors read fine, **yellow** if the last sample had a cartridge or LTR390 problem, and **red** for a storage problem.

Serial commands (type `help`): `status`, `list`, `dump [n]` (prints a session; sampling pauses while it runs), `delete <n|all>` (never the session being written; if logging stopped because flash filled up, that session can be deleted too), `format`, `interval <ms|0>` (saved across restarts; 0 restores the `config.h` default), and `live on|off`.

## Web app (Wi-Fi)

**On your Wi-Fi (preferred).** Copy `logins.h.example` to `logins.h` in this folder and fill in your network's name and password; git ignores `logins.h`. It uses the same `WIFI_SSID` / `WIFI_PASSWORD` names as Metal-Gear-Mk-0's `logins.h`, so that file works as is. The calibrator then joins your network with modem sleep (the radio naps between the router's beacons), which should draw much less than running a hotspot. Open **http://ojodelsol-calibrator.local** on a phone or computer on the same network.
- That `.local` name works on iPhone, Mac, Windows and Linux, and on most recent Android phones. If your phone can't open it, use the IP address instead. The IP is printed on serial at boot, and is also listed in your router's device list as `ojodelsol-calibrator`. Reserving that address in the router keeps it the same, so a bookmark keeps working.
- If the connection drops later, it keeps trying to reconnect. It does not switch to the hotspot mid-run.

**Hotspot (fallback).** If there is no `logins.h`, or the calibrator can't join your network within 20 seconds of power-up (`WIFI_STA_CONNECT_TIMEOUT_MS`), it creates its own network instead. That needs no router or internet:

1. Join the network **`OjoDelSol-Calibrator`** (password **`ojodelsol`**). **Change the password in `config.h`** (`WIFI_AP_PASSWORD`, 8–63 characters): anyone who knows it can read and delete your logs.
2. Open **http://192.168.4.1** (a "sign in to network" page should also pop up on most phones; `http://ojodelsol-calibrator.local` works here too). If downloads or uploads don't work in that pop-up window, open the address in your normal browser instead.

**Clock.** The device has no clock, so it needs the time from somewhere for sessions to get real timestamps:
- **On your Wi-Fi:** it asks `pool.ntp.org` for the time as soon as it connects. If that server doesn't answer, it tries `time.google.com` and then `time.cloudflare.com`. It re-syncs every 3 hours. The servers are `NTP_SERVER_1`–`3` in `config.h`.
- **On the hotspot, or before a server answers:** the page sends the phone's clock when it opens. Once internet time is in use, the page stops doing that automatically. **Sync clock from this device** on the Storage tab still works and shows which source set the clock.
- **Time zones:** timestamps are stored in UTC, and the page shows them in the viewing device's time zone, so there is no time zone to set.

| Tab | What it does |
|-----|--------------|
| **Live** | Current cartridge level and UV index, the bars each game (Boktai 1 J/U, Boktai 1 E, Boktai 2/3) would show, a calibration box (default 230, saved in the phone's browser), and a chart of the last few minutes |
| **History** | Pick any session on the device and chart it over time, with zoom sliders, summary numbers, and a **UV index vs cartridge** scatter plot with the game's bar thresholds and a rough estimate of the UV index at which the cartridge reaches each bar |
| **Storage** | Flash usage and an estimate of the time left; list, view, download and delete sessions; **backup** of all sessions as one `.tar`; **restore** from a `.tar` or individual `cal_NNNN.csv` files; change the sample interval; start a new session; set the clock; delete all old sessions; format |

Screenshots (rendered in a desktop browser from **mock data** with the page's real code; this is not a photo of the device):

| Live | History | Storage |
|------|---------|---------|
| <img src="screenshots/web-live-phone.png" width="260"> | <img src="screenshots/web-history-phone.png" width="260"> | <img src="screenshots/web-storage-phone.png" width="260"> |

Desktop layout: [Live](screenshots/web-live-desktop.png), [History](screenshots/web-history-desktop.png).

If a sensor is missing:
- Wi-Fi and the web app start regardless, and sampling carries on. The Live tab shows what is wrong: **"Cartridge: no answer"** when the cartridge's sensor flag never rises, **"Cartridge … bus read-back check failed"** when it did answer but the GPIO registers did not read back what was written (the reading may be noise), and **"LTR390: not found"** with the pins to check. The device retries a missing LTR390 every 30 seconds, so you can plug it in later.
- Rows are still logged with `cart_status = timeout` and/or `ltr_status = absent`, and the LED turns yellow.
- With **no cartridge at all** the bus pins float, so a stray reading is possible now and then; `cart_bus = 0` and the warning above are the reliable signs. This is reasoning from the code, not something I have seen on hardware.

Good to know:
- The web server runs on core 0, so sampling on core 1 should not stall while a page loads or a file downloads. **This is unverified**; Wi-Fi interrupts could in principle disturb the cartridge timing, so compare a run with `WIFI_ENABLED = false` if the readings look odd.
- Wi-Fi uses more power than sampling alone, the hotspot much more than your own network. Current draw is unmeasured; plan on USB power.
- Up to 4 devices can join the hotspot (`WIFI_AP_MAX_CLIENTS`). The server handles one request at a time, so a large download briefly pauses the live view.
- Large uploads and the backup run in your browser; the History tab downloads the whole session CSV to draw it, which can take a while for a multi-day session.
- The calibration number and chosen bar table are remembered by the phone's browser, not the device.
- Interface for scripts: `GET /api/status`, `/api/live?since=N`, `/api/files`, `/api/file?n=N` (CSV); `POST /api/delete?n=N|all=1`, `/api/newsession`, `/api/format?confirm=ERASE`, `/api/interval?ms=N`, `/api/time?ms=EPOCH_MS`, and a multipart `/api/upload`. See the header of `WebApp.ino`.

## Reading the data

The game turns the cartridge's raw value into a 0–140 level with `level = calibration − cart_raw` (anything below 0 becomes 0, anything above 140 becomes 140), then maps that level to bars. The game sets `calibration` during its sensor setup as the raw value it measures at that moment **minus 2** (presumably taken in the dark; I have not confirmed what the game asks you to do). So take the `cart_raw` the cartridge shows when the sensor is covered, subtract 2, and use that as the calibration. Plot `uvi` against the resulting level to see how the LTR390 maps onto the cartridge across a day of changing light.

**How the `uvi` column relates to the Ojo del Sol's own UV reading.** The calibrator sets up the LTR390 exactly like the main firmware (UV mode, 18x gain, 20-bit / about 400 ms, 500 ms measurement rate, `counts ÷ 2300`), so `uvi` is the same quantity as the main firmware's *UVI measured* value. The difference is what happens next: the main firmware then applies enclosure compensation (`UV_ENCLOSURE_TRANSMITTANCE`, currently 0.833, which multiplies the reading by about 1.2) and can optionally smooth it, while the calibrator logs the raw, uncorrected value and does no smoothing. The thresholds in the main firmware's [`config.h`](../config.h) (`AUTO_UV_MIN`, `AUTO_UV_SATURATION` and the per-game values) are **open-air** UV values, so the calibrator's bare-sensor `uvi` is the number to compare them with: if the Ojo del Sol sits behind a window, its *corrected* UVI should land near what the calibrator reads in the same sunlight. Neither device has been checked against a reference UV meter; both rely on the datasheet's nominal 2300 counts per UVI, which cancels out when you compare against the cartridge but means the absolute UV index values could be off. Different LTR390 chips, mounting angles and diffusers will also read a little differently, so use the same breakout and orientation if you want the closest match.

## Not verified yet

- Whether the bus timing and the resistor-tied lines work on the ESP32-S3 (tested method only on a GBxCart RW, which drives the full bus).
- Whether the sensor tolerates the game's ~750 µs tick on this setup (`CART_TICK_US` is adjustable).
- The `cart_bus` read-back check is a guess about how the GPIO registers answer, so a `0` there is not proof of a fault.
- Long-run behaviour: flash wear, the "out of space" handling, and recovery from a write error.
- Wi-Fi and the web server: joining your network, the `ojodelsol-calibrator.local` name, internet time, and serving the page and the status and live APIs work on the board. Untested: the hotspot fallback and its captive-portal pop-up, a phone (only a desktop PC was used), file downloads and uploads over the ESP32 `WebServer`, restore of large files, and the locking between the web task and the sampling loop under load.
- Whether Wi-Fi activity disturbs the cartridge read timing (the ticks are software-timed).
- A flash-size mismatch: `partitions.csv` assumes 16 MB of flash. (The test board has 16 MB, and its 12.9 MB log partition mounted fine.)
- Boktai 1 cartridges: the sequence follows the Boktai 2 and 3 code (identical in all four Boktai 2 versions and Boktai 3 (J)). Boktai 1's sensor code differs slightly: it does not initialise the clock/reset byte (so the clock likely starts low instead of high) and does not wait for the flag to be low before counting. I expect that to make its raw value one count higher than Boktai 2/3, which cancels out in `calibration − raw` when both come from the same game, but this is inferred from the ROMs and untested.
- Power: supply current is unmeasured. The cartridge and the LTR390 both run from the board's 3V3 pin. It is meant to run from USB power; there is no sleep mode or battery handling, so a lost supply ends the run (and loses up to the last 10 rows).

