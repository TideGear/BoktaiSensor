// TQTDisplay.h - LilyGO T-QT Pro built-in display support
//
// EXPERIMENTAL: the T-QT Pro build is currently untested on real hardware.
//
// The UI is authored at 128x64 (the SSD1306 OLED of the original build).
// This class renders it into an offscreen 1-bit canvas and pushes the result
// centered onto the T-QT's 128x128 GC9107 TFT, mirroring the subset of the
// Adafruit_SSD1306 API that BoktaiSensor.ino uses so all drawing code stays
// board-independent.
//
// Uses the "GFX Library for Arduino" (Arduino_GFX by moononournation), which
// has a native GC9107 driver and works with esp32 core 3.x. TFT_eSPI was
// deliberately avoided: its last release (2.5.43) crashes in tft.init() on
// ESP32-S3 with esp32 core 3.x (StoreProhibited panic -> boot loop).
// Pins below match Arduino_GFX's official LILYGO_T_QT_PRO device definition;
// no library config files need editing.
#ifndef TQT_DISPLAY_H
#define TQT_DISPLAY_H

#include <Adafruit_GFX.h>
#include <Arduino_GFX_Library.h>

// Avoid the library's color macros (renamed across Arduino_GFX versions)
const uint16_t TQT_RGB565_BLACK = 0x0000;
const uint16_t TQT_RGB565_WHITE = 0xFFFF;

class TQTDisplay : public GFXcanvas1 {
public:
  TQTDisplay(uint16_t w, uint16_t h) : GFXcanvas1(w, h) {
    lineBuf = new uint16_t[w];
    bus = new Arduino_ESP32SPI(6 /* DC */, 5 /* CS */, 3 /* SCK */, 2 /* MOSI */,
                               GFX_NOT_DEFINED /* MISO */);
    gfx = new Arduino_GC9107(bus, 1 /* RST */, 0 /* rotation */, true /* IPS */);
  }

  bool begin(uint8_t rotation) {
    if (!gfx->begin()) {
      return false;
    }
    gfx->setRotation(rotation);
    gfx->fillScreen(TQT_RGB565_BLACK);
    xOffset = ((int16_t)gfx->width() - (int16_t)width()) / 2;
    yOffset = ((int16_t)gfx->height() - (int16_t)height()) / 2;
    if (xOffset < 0) xOffset = 0;
    if (yOffset < 0) yOffset = 0;
    panelAsleep = false;
    return true;
  }

  void clearDisplay() {
    fillScreen(0);
  }

  // Push the canvas to the panel (equivalent of Adafruit_SSD1306::display()).
  void display() {
    const uint8_t* buf = getBuffer();
    int16_t w = width();
    int16_t h = height();
    int16_t bytesPerRow = (w + 7) / 8;
    for (int16_t row = 0; row < h; row++) {
      const uint8_t* src = buf + (row * bytesPerRow);
      for (int16_t col = 0; col < w; col++) {
        lineBuf[col] = (src[col >> 3] & (0x80 >> (col & 7))) ? TQT_RGB565_WHITE
                                                             : TQT_RGB565_BLACK;
      }
      gfx->draw16bitRGBBitmap(xOffset, yOffset + row, lineBuf, w, 1);
    }
  }

  // Blank the physical panel (letterbox areas included).
  void fillPanelBlack() {
    gfx->fillScreen(TQT_RGB565_BLACK);
  }

  // Panel sleep draws ~100uA; the backlight pin is handled by the sketch.
  void panelSleep() {
    if (panelAsleep) {
      return;
    }
    gfx->displayOff();
    panelAsleep = true;
  }

  // Returns true if the panel was actually asleep and had to be woken,
  // so callers can skip redundant clears on repeated calls.
  bool panelWake() {
    if (!panelAsleep) {
      return false;
    }
    gfx->displayOn();
    delay(120);  // Sleep-out settle time
    panelAsleep = false;
    return true;
  }

private:
  Arduino_DataBus* bus;
  Arduino_GC9107* gfx;
  uint16_t* lineBuf;
  int16_t xOffset = 0;
  int16_t yOffset = 0;
  bool panelAsleep = true;
};

#endif
