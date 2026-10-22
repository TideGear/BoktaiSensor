// GbaCartBus.h - read a Boktai cartridge's solar sensor over the GBA cartridge
// bus: the bus cycles and the game's sensor sequence, one tick at a time.
//
// Shared by the Ojo del Sol firmware (SENSOR_GBA_CARTRIDGE, via
// GbaCartSensor.h) and the Ojo del Sol Calibrator (OjoDelSolCalibrator/,
// which includes it as "../GbaCartBus.h"), so a fix here reaches both. Each
// sketch supplies the timing: call cartSequenceTick() once every
// CART_TICK_US. Include it after a config.h that defines CART_AD_PINS[6]
// (AD0, AD1, AD2, AD3, AD5, AD6), CART_CS_PIN, CART_RD_PIN and CART_WR_PIN.
//
// Method (checked against the Boktai 3 (J) ROM's sensor interrupt handler and
// against a GBxCart RW; NOT yet run on an ESP32): the cartridge exposes a GPIO
// port at 0x080000C4 (data), 0x080000C6 (direction) and 0x080000C8 (read
// enable).
//   data bit 0 = clock, bit 1 = reset, bit 3 = sensor flag (input)
// The game holds reset high for 10 ticks, releases it, then toggles the clock
// every tick. The sensor raises its flag once the number of clock pulses
// passes a threshold set by the light level, and the game's raw value is
// (ticks >> 1) at the tick the flag is first seen. Dark gives a large raw
// value (about 230), bright light gives a small one (0 = saturated).
// This is the Boktai 2/3 sequence; Boktai 1's differs slightly (see the
// calibrator README) and is used for all games here.
#ifndef GBA_CART_BUS_H
#define GBA_CART_BUS_H

#include <Arduino.h>
#include "driver/gpio.h"

// Halfword bus addresses (byte address >> 1) of the GPIO registers
static const uint16_t CART_REG_DATA = 0x62;  // 0x080000C4
static const uint16_t CART_REG_DIR = 0x63;   // 0x080000C6
static const uint16_t CART_REG_RE = 0x64;    // 0x080000C8

// Which bus bit each entry of CART_AD_PINS carries (AD4 is not wired to the ESP32)
static const uint8_t CART_AD_BITS[6] = { 0, 1, 2, 3, 5, 6 };

// The gpio_* calls below are much cheaper than pinMode(), which matters because
// every tick does two bus cycles. cartBusPinsBegin() must run first: it
// switches the pins to GPIO mode, which the gpio_* calls rely on.
static inline void cartDriveBits(uint16_t value) {
  for (int i = 0; i < 6; i++) {
    gpio_num_t pin = (gpio_num_t)CART_AD_PINS[i];
    gpio_set_level(pin, (value >> CART_AD_BITS[i]) & 1);
    gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT);
  }
}

static inline void cartReleaseBits() {
  for (int i = 0; i < 6; i++) gpio_set_direction((gpio_num_t)CART_AD_PINS[i], GPIO_MODE_INPUT);
}

// Address/data lines released, control lines idle high
static void cartBusPinsBegin() {
  const uint8_t controlPins[3] = { CART_CS_PIN, CART_RD_PIN, CART_WR_PIN };
  for (int i = 0; i < 3; i++) {
    digitalWrite(controlPins[i], HIGH);
    pinMode(controlPins[i], OUTPUT);
    digitalWrite(controlPins[i], HIGH);
  }
  for (int i = 0; i < 6; i++) pinMode(CART_AD_PINS[i], INPUT);
}

// Every bus pin floating
static void cartBusPinsRelease() {
  for (int i = 0; i < 6; i++) pinMode(CART_AD_PINS[i], INPUT);
  pinMode(CART_CS_PIN, INPUT);
  pinMode(CART_RD_PIN, INPUT);
  pinMode(CART_WR_PIN, INPUT);
}

// One bus write cycle: address latched on /CS falling, data strobed by /WR.
static void cartBusWrite(uint16_t halfwordAddr, uint8_t value) {
  cartDriveBits(halfwordAddr);
  gpio_set_level((gpio_num_t)CART_CS_PIN, 0);
  delayMicroseconds(1);
  cartDriveBits(value);
  gpio_set_level((gpio_num_t)CART_WR_PIN, 0);
  delayMicroseconds(1);
  gpio_set_level((gpio_num_t)CART_WR_PIN, 1);
  gpio_set_level((gpio_num_t)CART_CS_PIN, 1);
  cartReleaseBits();
}

// One bus read cycle. Returns the bits the ESP32 can see (0-3, 5, 6).
static uint8_t cartBusRead(uint16_t halfwordAddr) {
  cartDriveBits(halfwordAddr);
  gpio_set_level((gpio_num_t)CART_CS_PIN, 0);
  delayMicroseconds(1);
  cartReleaseBits();
  gpio_set_level((gpio_num_t)CART_RD_PIN, 0);
  delayMicroseconds(1);
  uint8_t value = 0;
  for (int i = 0; i < 6; i++) {
    if (gpio_get_level((gpio_num_t)CART_AD_PINS[i])) value |= (1 << CART_AD_BITS[i]);
  }
  gpio_set_level((gpio_num_t)CART_RD_PIN, 1);
  gpio_set_level((gpio_num_t)CART_CS_PIN, 1);
  return value;
}

// State of one solar sensor reading in progress
struct CartSequence {
  uint8_t data;       // value written to the data register each tick
  int state;          // 0 = reset/settle, 1 = counting
  uint32_t counter;   // ticks
  uint32_t guard;     // total ticks, to bound the reading
  bool ok;            // flag seen before the timeout (valid once finished)
  uint16_t raw;       // game raw value (valid when ok)
};

// Start a reading. Returns whether the GPIO data register read back what was
// written (a rough wiring check). Call cartSequenceTick() one tick later.
static bool cartSequenceBegin(CartSequence* seq) {
  *seq = { 0, 0, 0, 0, false, 0 };

  cartBusWrite(CART_REG_RE, 1);    // map the GPIO port into the ROM area
  cartBusWrite(CART_REG_DIR, 7);   // bits 0-2 output, bit 3 (flag) input

  // Initial state used by the game: clock high, reset low
  seq->data = 1;
  cartBusWrite(CART_REG_DATA, seq->data);
  return (cartBusRead(CART_REG_DATA) & 0x07) == (seq->data & 0x07);
}

// One sensor tick, following the game's own sequence. Returns true when the
// reading has finished (seq->ok and seq->raw are then set) and the bus has
// been returned to normal ROM reads.
static bool cartSequenceTick(CartSequence* seq) {
  bool flag = (cartBusRead(CART_REG_DATA) & 0x08) != 0;  // read before write, like the game
  cartBusWrite(CART_REG_DATA, seq->data);

  bool finished = false;
  if (seq->state == 0) {
    if (seq->counter <= 9) seq->data |= 0x02; else seq->data &= (uint8_t)~0x02;
    if (seq->counter > 0x14 && !flag) {
      seq->state = 1;
      seq->counter = 0;
    }
    seq->counter++;
  } else if (flag) {
    seq->ok = true;
    seq->raw = (uint16_t)(seq->counter >> 1);
    finished = true;
  } else {
    seq->data ^= 0x01;
    if (seq->counter > 0x1FF) {
      finished = true;  // never saw the flag
    } else {
      seq->counter++;
    }
  }
  if (++seq->guard >= 1024) {
    finished = true;
  }

  if (finished) {
    cartBusWrite(CART_REG_RE, 0);  // back to normal ROM reads
  }
  return finished;
}

#endif
