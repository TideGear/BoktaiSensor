// GbaCartSensor.h - read a Boktai cartridge's solar sensor over the GBA
// cartridge bus (SENSOR_GBA_CARTRIDGE builds only)
//
// UNTESTED on real hardware. The bus cycles and the game's sensor sequence
// live in GbaCartBus.h, shared with the Ojo del Sol Calibrator; see that file
// for how the reading works. This file supplies the timing, the cartridge
// power switch and the interface loop() uses.
//
// Timing differs from the calibrator: it busy-waits through a whole reading
// (up to ~0.4 s), which here would stall the buttons, display and HID output.
// Instead each tick runs from a hardware timer interrupt, like the game's own
// timer interrupt, and loop() only starts readings and collects results
// through cartPoll().
//
// The interrupt is allocated on the core that calls cartBegin() (core 1, the
// Arduino loop core), so the Bluetooth stack and controller (core 0) cannot
// delay it. Flash writes (e.g. storing a Bluetooth pairing) still pause it,
// so every reading records its latest tick, and readings with a tick later
// than CART_TICK_MAX_LATE_US can be discarded.
#ifndef GBA_CART_SENSOR_H
#define GBA_CART_SENSOR_H

#include <Arduino.h>
#include "esp_timer.h"
#include "config.h"
#include "GbaCartBus.h"

struct CartReading {
  bool ok;              // flag seen before the timeout
  uint16_t raw;         // game raw value (valid when ok)
  bool busOk;           // GPIO register read-back matched what was written
  uint32_t durationMs;
  uint32_t maxLateUs;   // how much later than CART_TICK_US the latest tick came
};

enum CartPhase : uint8_t {
  CART_IDLE = 0,
  CART_RUNNING,
  CART_DONE
};

static hw_timer_t* cartTimer = nullptr;
static portMUX_TYPE cartMux = portMUX_INITIALIZER_UNLOCKED;
static volatile CartPhase cartPhase = CART_IDLE;
static bool cartEverStarted = false;
static unsigned long cartLastStartMs = 0;

// Reading state, only touched by the timer interrupt while a reading runs
static CartSequence cartSeq;
static CartReading cartResult = { false, 0, false, 0, 0 };
static unsigned long cartStartMs = 0;
static int64_t cartLastTickUs = 0;

static void ARDUINO_ISR_ATTR cartTickIsr() {
  if (cartPhase != CART_RUNNING) {
    return;
  }

  int64_t nowUs = esp_timer_get_time();
  int64_t lateUs = (nowUs - cartLastTickUs) - (int64_t)CART_TICK_US;
  if (lateUs > (int64_t)cartResult.maxLateUs) {
    cartResult.maxLateUs = (uint32_t)lateUs;
  }
  cartLastTickUs = nowUs;

  if (cartSequenceTick(&cartSeq)) {
    cartResult.ok = cartSeq.ok;
    cartResult.raw = cartSeq.raw;
    cartResult.durationMs = millis() - cartStartMs;
    portENTER_CRITICAL_ISR(&cartMux);
    cartPhase = CART_DONE;
    portEXIT_CRITICAL_ISR(&cartMux);
  }
}

static void cartStartReading() {
  cartResult = { false, 0, false, 0, 0 };
  cartStartMs = millis();
  cartResult.busOk = cartSequenceBegin(&cartSeq);

  cartLastTickUs = esp_timer_get_time();
  portENTER_CRITICAL(&cartMux);
  cartPhase = CART_RUNNING;
  portEXIT_CRITICAL(&cartMux);
  timerWrite(cartTimer, 0);
  timerStart(cartTimer);
}

// Stop reading, float the bus and cut the cartridge's power (when
// CART_POWER_PIN is wired). Used at boot, before a restart and before deep
// sleep. The switch's gate pull-up keeps the cartridge off during deep sleep;
// without a switch, pull-ups on /CS, /RD and /WR keep it idle instead.
static void cartEnd() {
  portENTER_CRITICAL(&cartMux);
  cartPhase = CART_IDLE;
  portEXIT_CRITICAL(&cartMux);
  if (cartTimer != nullptr) {
    // The interrupt runs on this core, so no tick can be half done here.
    timerStop(cartTimer);
  }
  cartBusPinsRelease();
  if (CART_POWER_PIN >= 0) {
    // Bus released first, so the ESP32 never drives an unpowered cartridge.
    digitalWrite(CART_POWER_PIN, HIGH);  // Active LOW: HIGH = off
    pinMode(CART_POWER_PIN, OUTPUT);
    digitalWrite(CART_POWER_PIN, HIGH);
  }
}

// Power the cartridge, set up the bus pins (control lines idle high) and the
// tick timer. Call from the core that runs loop().
static bool cartBegin() {
  cartEnd();  // Known state: bus floating, cartridge off
  if (CART_POWER_PIN >= 0) {
    digitalWrite(CART_POWER_PIN, LOW);  // Active LOW: LOW = on
    delay(CART_POWER_ON_MS);
  }
  cartBusPinsBegin();

  if (cartTimer == nullptr) {
    cartTimer = timerBegin(1000000);  // 1 MHz: one count per microsecond
    if (cartTimer == nullptr) {
      return false;
    }
    timerStop(cartTimer);
    timerAttachInterrupt(cartTimer, cartTickIsr);
    timerAlarm(cartTimer, CART_TICK_US, true, 0);  // Auto-reload, unlimited
  }
  cartPhase = CART_IDLE;
  cartEverStarted = false;
  return true;
}

// Call from loop(). Starts a reading every CART_READ_INTERVAL_MS and returns
// true, filling *out, when one has finished.
static bool cartPoll(CartReading* out) {
  if (cartTimer == nullptr) {
    return false;
  }

  portENTER_CRITICAL(&cartMux);
  CartPhase phase = cartPhase;
  portEXIT_CRITICAL(&cartMux);

  if (phase == CART_DONE) {
    timerStop(cartTimer);
    *out = cartResult;
    cartPhase = CART_IDLE;
    return true;
  }

  unsigned long now = millis();
  if (phase == CART_IDLE && (!cartEverStarted || (now - cartLastStartMs) >= CART_READ_INTERVAL_MS)) {
    cartEverStarted = true;
    cartLastStartMs = now;
    cartStartReading();
  }
  return false;
}

#endif
