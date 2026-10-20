// GbaCartSensor.h - read a Boktai cartridge's solar sensor over the GBA cartridge bus
//
// Method (checked against the Boktai 3 (J) ROM's sensor interrupt handler and
// against a GBxCart RW; NOT yet run on an ESP32): the cartridge exposes a GPIO port at
// 0x080000C4 (data), 0x080000C6 (direction) and 0x080000C8 (read enable).
//   data bit 0 = clock, bit 1 = reset, bit 3 = sensor flag (input)
// The game holds reset high for 10 ticks, releases it, then toggles the clock
// every tick. The sensor raises its flag once the number of clock pulses passes
// a threshold set by the light level, and the game's raw value is (ticks >> 1)
// at the tick the flag is first seen. Dark gives a large raw value (about 230),
// bright light gives a small one (0 = saturated).
#ifndef GBA_CART_SENSOR_H
#define GBA_CART_SENSOR_H

#include <Arduino.h>
#include "esp_timer.h"
#include "config.h"

// Halfword bus addresses (byte address >> 1) of the GPIO registers
static const uint16_t CART_REG_DATA = 0x62;  // 0x080000C4
static const uint16_t CART_REG_DIR = 0x63;   // 0x080000C6
static const uint16_t CART_REG_RE = 0x64;    // 0x080000C8

// Which bus bit each entry of CART_AD_PINS carries (AD4 is not wired to the ESP32)
static const uint8_t CART_AD_BITS[6] = { 0, 1, 2, 3, 5, 6 };

struct CartReading {
  bool ok;            // flag seen before the timeout
  uint16_t raw;       // game raw value (valid when ok)
  bool busOk;         // GPIO register read-back matched what was written
  uint32_t durationMs;
};

static inline void cartDriveBits(uint16_t value) {
  for (int i = 0; i < 6; i++) {
    pinMode(CART_AD_PINS[i], OUTPUT);
    digitalWrite(CART_AD_PINS[i], (value >> CART_AD_BITS[i]) & 1);
  }
}

static inline void cartReleaseBits() {
  for (int i = 0; i < 6; i++) pinMode(CART_AD_PINS[i], INPUT);
}

static void cartBusBegin() {
  digitalWrite(CART_CS_PIN, HIGH);
  digitalWrite(CART_RD_PIN, HIGH);
  digitalWrite(CART_WR_PIN, HIGH);
  pinMode(CART_CS_PIN, OUTPUT);
  pinMode(CART_RD_PIN, OUTPUT);
  pinMode(CART_WR_PIN, OUTPUT);
  digitalWrite(CART_CS_PIN, HIGH);
  digitalWrite(CART_RD_PIN, HIGH);
  digitalWrite(CART_WR_PIN, HIGH);
  cartReleaseBits();
}

// One bus write cycle: address latched on /CS falling, data strobed by /WR.
static void cartBusWrite(uint16_t halfwordAddr, uint8_t value) {
  cartDriveBits(halfwordAddr);
  digitalWrite(CART_CS_PIN, LOW);
  delayMicroseconds(1);
  cartDriveBits(value);
  digitalWrite(CART_WR_PIN, LOW);
  delayMicroseconds(1);
  digitalWrite(CART_WR_PIN, HIGH);
  digitalWrite(CART_CS_PIN, HIGH);
  cartReleaseBits();
}

// One bus read cycle. Returns the bits the ESP32 can see (0-3, 5, 6).
static uint8_t cartBusRead(uint16_t halfwordAddr) {
  cartDriveBits(halfwordAddr);
  digitalWrite(CART_CS_PIN, LOW);
  delayMicroseconds(1);
  cartReleaseBits();
  digitalWrite(CART_RD_PIN, LOW);
  delayMicroseconds(1);
  uint8_t value = 0;
  for (int i = 0; i < 6; i++) {
    if (digitalRead(CART_AD_PINS[i])) value |= (1 << CART_AD_BITS[i]);
  }
  digitalWrite(CART_RD_PIN, HIGH);
  digitalWrite(CART_CS_PIN, HIGH);
  return value;
}

// Take one solar sensor reading, following the game's own sequence tick by tick.
static CartReading cartReadSolarSensor() {
  CartReading result = { false, 0, false, 0 };
  uint32_t startMs = millis();

  cartBusWrite(CART_REG_RE, 1);    // map the GPIO port into the ROM area
  cartBusWrite(CART_REG_DIR, 7);   // bits 0-2 output, bit 3 (flag) input

  // Initial state used by the game: clock high, reset low
  uint8_t data = 1;
  cartBusWrite(CART_REG_DATA, data);
  result.busOk = ((cartBusRead(CART_REG_DATA) & 0x07) == (data & 0x07));

  int state = 0;          // 0 = reset/settle, 1 = counting
  uint32_t counter = 0;   // ticks
  int64_t nextTick = esp_timer_get_time();

  for (uint32_t guard = 0; guard < 1024; guard++) {
    while (esp_timer_get_time() < nextTick) { }
    nextTick += CART_TICK_US;

    bool flag = (cartBusRead(CART_REG_DATA) & 0x08) != 0;  // read before write, like the game
    cartBusWrite(CART_REG_DATA, data);

    if (state == 0) {
      if (counter <= 9) data |= 0x02; else data &= (uint8_t)~0x02;
      if (counter > 0x14 && !flag) {
        state = 1;
        counter = 0;
      }
      counter++;
    } else {
      if (flag) {
        result.ok = true;
        result.raw = (uint16_t)(counter >> 1);
        break;
      }
      data ^= 0x01;
      if (counter > 0x1FF) break;  // never saw the flag
      counter++;
    }
  }

  cartBusWrite(CART_REG_RE, 0);    // back to normal ROM reads
  result.durationMs = millis() - startMs;
  return result;
}

#endif
