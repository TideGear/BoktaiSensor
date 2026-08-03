// EPaperDisplay.h - LilyGO Mini E-Paper S3 built-in display support
//
// The UI is authored at 128x64 monochrome (inherited from the original
// SSD1306 OLED build). This class renders it into an offscreen 1-bit canvas
// and pushes the result onto the board's 1.02" GDEW0102T4 panel, mirroring
// the subset of the Adafruit_SSD1306 API that BoktaiSensor.ino uses so all
// drawing code stays display-independent.
//
// The panel is 80x128 native (portrait). Rotation 1 or 3 turns it into
// 128x80 landscape, which the 128x64 canvas is centered inside (8 blank
// rows top and bottom). Set pixels become black ink on a white background,
// the inverse of the OLED, so the artwork reads correctly on paper.
//
// Uses GxEPD2 (ZinggJM), which has a native GDEW0102T4 / UC8175 driver with
// fast partial update and works with esp32 core 3.x. Install "GxEPD2" from
// the Arduino Library Manager; no library config files need editing.
//
// E-paper is slow (a refresh is hundreds of milliseconds) and wears with
// every refresh, so unlike an OLED this class does not push a frame on every
// display() call. It:
//   - skips pushes whose pixels are identical to what is already on the panel,
//   - rate-limits pushes to EPAPER_MIN_REFRESH_MS, deferring the newest frame
//     until the interval elapses (flushed by tick() from loop()),
//   - promotes every EPAPER_FULL_REFRESH_EVERY'th push to a full refresh to
//     clear the ghosting that repeated partial refreshes leave behind.
// displayNow() bypasses the rate limit for one-shot status screens.
#ifndef EPAPER_DISPLAY_H
#define EPAPER_DISPLAY_H

#include <Adafruit_GFX.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include "driver/gpio.h"
#include "config.h"

class EPaperDisplay : public GFXcanvas1 {
public:
  EPaperDisplay(uint16_t w, uint16_t h)
    : GFXcanvas1(w, h),
      epd(GxEPD2_102(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN)) {
    frameBytes = (size_t)((w + 7) / 8) * h;
    lastFrame = (uint8_t*)malloc(frameBytes);
  }

  bool begin(uint8_t rotation) {
    if (lastFrame == nullptr) {
      return false;
    }
    // A previous deep sleep may have latched the power pin low.
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis((gpio_num_t)EPD_POWER_PIN);
    powerRailOn();

    SPI.end();
    SPI.begin(EPD_SCLK_PIN, EPD_MISO_PIN, EPD_MOSI_PIN, EPD_CS_PIN);

    // Bitrate 0: never let GxEPD2 open Serial. Whether Serial exists at all
    // depends on the USB mode the sketch chose (XInput vs CDC).
    epd.init(0, true, 20, false);
    epd.setRotation(landscapeRotation(rotation));
    if (epd.width() < (int16_t)width() || epd.height() < (int16_t)height()) {
      return false;
    }
    xOffset = (epd.width() - (int16_t)width()) / 2;
    yOffset = (epd.height() - (int16_t)height()) / 2;

    // Nothing has been pushed yet, so no previous frame to diff against.
    memset(lastFrame, 0xFF, frameBytes);
    hibernating = false;
    powered = true;
    forceFull = true;
    pending = false;
    pushedOnce = false;
    partialsSinceFull = 0;
    lastPushMs = 0;
    return true;
  }

  void clearDisplay() {
    fillScreen(0);
  }

  // Routine push, held to EPAPER_MIN_REFRESH_MS. If the interval has not
  // elapsed the frame is kept and flushed by a later tick()/display() call.
  void display() {
    pushFrame(EPAPER_MIN_REFRESH_MS);
  }

  // Push for content the user is waiting on — a changed bar count, or a screen
  // change they just pressed a button for. Held only to
  // EPAPER_PRIORITY_MIN_REFRESH_MS, which defaults to no floor at all.
  void displayPriority() {
    pushFrame(EPAPER_PRIORITY_MIN_REFRESH_MS);
  }

  // Unconditional push, for one-shot status screens that must appear right
  // away. Frames identical to what is on the panel are still skipped.
  void displayNow() {
    pushFrame(0);
  }

  // Called every loop iteration to flush a held frame once its interval has
  // elapsed. Uses the shortest interval any deferred caller asked for, so a
  // priority frame is never held back by a routine one.
  void tick() {
    if (pending) {
      pushFrame(pendingMinMs);
    }
  }

  // Hibernate the controller (~1uA, image retained) and cut the panel rail,
  // latching it off through deep sleep. E-paper keeps its last image unpowered,
  // so whatever was pushed last stays on screen — callers push a blank frame
  // first if they want the panel to look off.
  void powerDownForSleep() {
    hibernatePanel();
    digitalWrite(EPD_POWER_PIN, EPD_POWER_ACTIVE_HIGH ? LOW : HIGH);
    gpio_hold_en((gpio_num_t)EPD_POWER_PIN);
    gpio_deep_sleep_hold_en();
    powered = false;
  }

  // Restore the rail and controller after powerDownForSleep().
  // Returns true if the panel actually had to be woken.
  bool wakePanel() {
    if (powered && !hibernating) {
      return false;
    }
    bool railWasOff = !powered;
    if (railWasOff) {
      gpio_deep_sleep_hold_dis();
      gpio_hold_dis((gpio_num_t)EPD_POWER_PIN);
      powerRailOn();
      powered = true;
    }
    // initial = false: do not force GxEPD2's one-time clear-to-white.
    epd.init(0, false, 20, false);
    epd.setRotation(landscapeRotation(EPAPER_DISPLAY_ROTATION));
    hibernating = false;
    // Controller deep sleep loses the previous-image RAM that a differential
    // (partial) refresh diffs against, so the first push back must be full.
    forceFull = true;
    if (railWasOff) {
      // Unpowered, the panel kept showing whatever was pushed last, but we can
      // no longer assume it matches our copy. Treat it as unknown.
      memset(lastFrame, 0xFF, frameBytes);
    }
    return true;
  }

  // Request that the next push be a full refresh rather than a partial one.
  void requestFullRefresh() {
    forceFull = true;
  }

  // Frames actually pushed to the panel since boot. Lets callers pace work to
  // the panel's real refresh rate instead of guessing at it or running at loop
  // speed. Used by the no-sensor bar sweep.
  uint32_t pushCount() const {
    return pushes;
  }

private:
  // The 128-wide canvas only fits across the panel's long axis, so portrait
  // rotations are not usable. Snap them to the nearest landscape rotation.
  static uint8_t landscapeRotation(uint8_t rotation) {
    return ((rotation % 4) == 3) ? 3 : 1;
  }

  // Controller to deep sleep with the image retained. Any later push wakes it.
  void hibernatePanel() {
    if (!powered || hibernating) {
      return;
    }
    epd.hibernate();
    hibernating = true;
  }

  void powerRailOn() {
    pinMode(EPD_POWER_PIN, OUTPUT);
    digitalWrite(EPD_POWER_PIN, EPD_POWER_ACTIVE_HIGH ? HIGH : LOW);
    delay(10);  // Let the panel rail settle before the reset pulse
  }

  void pushFrame(unsigned long minIntervalMs) {
    if (lastFrame == nullptr) {
      return;
    }
    if (memcmp(lastFrame, getBuffer(), frameBytes) == 0) {
      pending = false;  // Already on the panel
      return;
    }

    unsigned long now = millis();
    if (minIntervalMs > 0 && pushedOnce && (now - lastPushMs) < minIntervalMs) {
      // Hold this frame for tick() to flush. Keep the shortest interval asked
      // for so a routine push cannot delay a priority one already waiting.
      pendingMinMs = pending ? min(pendingMinMs, minIntervalMs) : minIntervalMs;
      pending = true;
      return;
    }

    wakePanel();

    bool full = forceFull || !pushedOnce ||
                (EPAPER_FULL_REFRESH_EVERY > 0 &&
                 partialsSinceFull >= EPAPER_FULL_REFRESH_EVERY);
    if (full) {
      epd.setFullWindow();
      partialsSinceFull = 0;
      forceFull = false;
    } else {
      epd.setPartialWindow(0, 0, epd.width(), epd.height());
      partialsSinceFull++;
    }

    epd.firstPage();
    do {
      epd.fillScreen(GxEPD_WHITE);
      // Set canvas bits are ink; the letterbox rows stay white.
      epd.drawBitmap(xOffset, yOffset, getBuffer(), width(), height(),
                     GxEPD_BLACK, GxEPD_WHITE);
    } while (epd.nextPage());

    memcpy(lastFrame, getBuffer(), frameBytes);
    lastPushMs = millis();
    pushedOnce = true;
    pending = false;
    pushes++;
  }

  // Page height = panel height, so a frame is always a single page.
  GxEPD2_BW<GxEPD2_102, GxEPD2_102::HEIGHT> epd;
  uint8_t* lastFrame = nullptr;
  size_t frameBytes = 0;
  int16_t xOffset = 0;
  int16_t yOffset = 0;
  unsigned long lastPushMs = 0;
  unsigned long pendingMinMs = 0;
  uint32_t pushes = 0;
  uint16_t partialsSinceFull = 0;
  bool pending = false;
  bool pushedOnce = false;
  bool forceFull = true;
  bool hibernating = true;
  bool powered = false;
};

#endif
