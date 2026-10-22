// GbaCartSensor.h - read a Boktai cartridge's solar sensor over the GBA cartridge bus
//
// The bus cycles and the game's sensor sequence live in ../GbaCartBus.h, shared
// with the main Ojo del Sol firmware; see that file for how the reading works.
// This file only supplies the calibrator's timing: it busy-waits through a whole
// reading, ticking every CART_TICK_US.
#ifndef GBA_CART_SENSOR_H
#define GBA_CART_SENSOR_H

#include <Arduino.h>
#include "esp_timer.h"
#include "config.h"
#include "../GbaCartBus.h"

struct CartReading {
  bool ok;            // flag seen before the timeout
  uint16_t raw;       // game raw value (valid when ok)
  bool busOk;         // GPIO register read-back matched what was written
  uint32_t durationMs;
};

static void cartBusBegin() {
  cartBusPinsBegin();
}

// Take one solar sensor reading, following the game's own sequence tick by tick.
static CartReading cartReadSolarSensor() {
  CartReading result = { false, 0, false, 0 };
  uint32_t startMs = millis();

  CartSequence seq;
  result.busOk = cartSequenceBegin(&seq);

  int64_t nextTick = esp_timer_get_time();
  bool finished = false;
  while (!finished) {
    while (esp_timer_get_time() < nextTick) { }
    nextTick += CART_TICK_US;
    finished = cartSequenceTick(&seq);
  }

  result.ok = seq.ok;
  result.raw = seq.raw;
  result.durationMs = millis() - startMs;
  return result;
}

#endif
