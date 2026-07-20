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
// Requires the TFT_eSPI library configured with the bundled
// Setup211_LilyGo_T_QT_Pro_S3.h user setup (see README).
#ifndef TQT_DISPLAY_H
#define TQT_DISPLAY_H

#include <Adafruit_GFX.h>
#include <TFT_eSPI.h>

// GC9107 panel commands (same values as ST77xx/GC9A01)
const uint8_t TQT_CMD_SLPIN = 0x10;
const uint8_t TQT_CMD_SLPOUT = 0x11;
const uint8_t TQT_CMD_DISPOFF = 0x28;
const uint8_t TQT_CMD_DISPON = 0x29;

class TQTDisplay : public GFXcanvas1 {
public:
  TQTDisplay(uint16_t w, uint16_t h) : GFXcanvas1(w, h) {
    lineBuf = new uint16_t[w];
  }

  bool begin(uint8_t rotation) {
    tft.init();
    tft.setRotation(rotation);
    tft.fillScreen(TFT_BLACK);
    xOffset = ((int16_t)tft.width() - (int16_t)width()) / 2;
    yOffset = ((int16_t)tft.height() - (int16_t)height()) / 2;
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
        lineBuf[col] = (src[col >> 3] & (0x80 >> (col & 7))) ? TFT_WHITE : TFT_BLACK;
      }
      tft.pushImage(xOffset, yOffset + row, w, 1, lineBuf);
    }
  }

  // Panel sleep draws ~100uA; the backlight pin is handled by the sketch.
  void panelSleep() {
    if (panelAsleep) {
      return;
    }
    tft.writecommand(TQT_CMD_DISPOFF);
    tft.writecommand(TQT_CMD_SLPIN);
    panelAsleep = true;
  }

  // Returns true if the panel was actually asleep and had to be woken,
  // so callers can skip redundant clears on repeated calls.
  bool panelWake() {
    if (!panelAsleep) {
      return false;
    }
    tft.writecommand(TQT_CMD_SLPOUT);
    delay(120);  // Datasheet sleep-out settle time
    tft.writecommand(TQT_CMD_DISPON);
    panelAsleep = false;
    return true;
  }

  TFT_eSPI tft;

private:
  uint16_t* lineBuf;
  int16_t xOffset = 0;
  int16_t yOffset = 0;
  bool panelAsleep = true;
};

#endif
