======================================================================
UV METER PROJECT: WAVESHARE ESP32-S3 MINI + LTR390 + OLED
======================================================================

1. HARDWARE CONNECTIONS (I2C)
----------------------------------------------------------------------
Both the LTR390 sensor and the OLED display share the I2C bus.
Use the Waveshare ESP32-S3 Mini Pinout (GP8 and GP9).

| Component Pin | ESP32-S3 Mini Pin | Description            |
| :------------ | :---------------- | :--------------------- |
| OLED/LTR VIN  | 3V3 (OUT)         | 3.3V Power             |
| OLED/LTR GND  | GND               | Common Ground          |
| OLED/LTR SDA  | GP8               | I2C Data               |
| OLED/LTR SCL  | GP9               | I2C Clock              |
| Battery Red   | 5V                | Positive (+) Term      |
| Battery Black | GND               | Negative (-) Term      |

IMPORTANT: The 5V pin on this board is the battery input/charging pin. 
Connecting your 3.7V 450mAh LiPo here allows the board to charge the 
battery via USB-C and power itself when unplugged.

2. POWER BUTTON WIRING
----------------------------------------------------------------------
A tactile push button is used to wake/sleep the device.

| Button Pin    | ESP32-S3 Mini Pin | Description            |
| :------------ | :---------------- | :--------------------- |
| Pin 1 (leg A) | GP2               | Signal (internal pull-up) |
| Pin 2 (leg B) | GND               | Ground                 |

                 ESP32-S3 Mini
                 ┌──────────┐
                 │          │
    GP2 ─────────┤          │
                 │          │
                ┌┴┐         │
    Button      │ │ (NO)    │
                └┬┘         │
                 │          │
    GND ─────────┴──────────┘

NOTE: The push button has 4 pins arranged in a square. Pins on the 
same side are internally connected. Connect diagonally opposite pins
OR pins on opposite sides to use the normally-open (NO) switch.

POWER BUTTON BEHAVIOR:
- Hold button 3 seconds: Powers device ON from sleep
- Hold button 3 seconds: Powers device OFF (enters deep sleep)

3. LIBRARIES REQUIRED
----------------------------------------------------------------------
Install these via the Arduino Library Manager:
- Adafruit LTR390 Library
- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit BusIO

4. SETUP & USAGE
----------------------------------------------------------------------
- Open 'BoktaiSensor.ino' in the Arduino IDE.
- Ensure 'config.h' is in the same folder.
- Select Board: "ESP32S3 Dev Module" (or "Waveshare ESP32-S3-Zero").
- The LTR390 has a peak UV response between 300 and 350nm.
- Charging: Plug USB-C into the board to charge the LiPo. 
- Meters: The 8-segment and 10-segment bars represent the range 
  between UV_MIN and UV_MAX set in config.h.

5. POWER MANAGEMENT
----------------------------------------------------------------------
Deep sleep draws approximately 10µA, extending battery life significantly.
- To power ON: Hold button for 3 seconds
- To power OFF: Hold button for 3 seconds
- Timing is configurable via LONG_PRESS_MS in config.h
======================================================================