Single Analog Mode — Guide for Emulator Developers
======================================================================

## Overview

Single Analog Mode encodes the solar sensor bar count as a proportional deflection on a single analog stick axis. This is simpler and more direct than Incremental Mode (which requires tracking button press sequences) and works naturally with both the Ojo del Sol and a regular gamepad.

## How it works

The controller sends two simultaneous inputs:

1. **Meter unlock button** (default: R3) — held down the entire time the meter should be adjustable. The emulator should **only** update the in-game solar meter while this button is held. This prevents normal gameplay stick movement from accidentally changing the meter.
2. **Analog axis** (default: Right Stick X+) — the deflection amount encodes the bar count.

The 0.0–1.0 range of the axis is divided into equal bands based on the game's bar count. The emulator reads the current deflection, normalizes it to 0.0–1.0 for the selected axis direction, determines which band it falls into, and sets the in-game meter to that bar count.

## Band mapping

**Boktai 1** (8-bar gauge — 9 bands, one per level 0–8):

| Bars | Axis range |
|------|------------|
| 0 | 0.00 – 0.11 |
| 1 | 0.12 – 0.22 |
| 2 | 0.23 – 0.33 |
| 3 | 0.34 – 0.44 |
| 4 | 0.45 – 0.55 |
| 5 | 0.56 – 0.66 |
| 6 | 0.67 – 0.77 |
| 7 | 0.78 – 0.88 |
| 8 | 0.89 – 1.00 |

**Boktai 2 & 3** (10-bar gauge — 11 bands, one per level 0–10):

| Bars | Axis range |
|------|------------|
| 0  | 0.00 – 0.09 |
| 1  | 0.10 – 0.18 |
| 2  | 0.19 – 0.27 |
| 3  | 0.28 – 0.36 |
| 4  | 0.37 – 0.45 |
| 5  | 0.46 – 0.54 |
| 6  | 0.55 – 0.63 |
| 7  | 0.64 – 0.72 |
| 8  | 0.73 – 0.81 |
| 9  | 0.82 – 0.90 |
| 10 | 0.91 – 1.00 |

**Note:** The axis ranges above are rounded for readability. The actual band boundaries fall at exact multiples of `1 / num_levels` (1/9 for Boktai 1, 1/11 for Boktai 2 & 3). Use the `floor()` formula below for precise mapping after normalizing the raw axis input.

The Ojo del Sol sends the midpoint of each band (e.g. 5 bars on Boktai 1 = 0.61). This means even 0 bars is sent as a small positive deflection (the midpoint of band 0), not exact center. The emulator should use `floor(normalized_axis * num_levels)` clamped to the max bar count.

## Pseudocode

```
// On each frame / input poll:
if (gamepad.isPressed(UNLOCK_BUTTON)) {          // default: R3
    float raw = gamepad.getRightStickX();        // API-specific signed axis value
    float raw_norm = normalizeSignedAxis(raw);   // convert API range to -1.0..1.0
    float axis_sign = +1.0;                      // +1 for X+/Y+, -1 for X-/Y-
    float signed_axis = raw_norm * axis_sign;
    float normalized = clamp((signed_axis + 1.0) * 0.5, 0.0, 1.0);
    int num_levels = game_max_bars + 1;          // 9 for Boktai 1, 11 for Boktai 2/3
    int bars = clamp(floor(normalized * num_levels), 0, game_max_bars);
    setSolarMeter(bars);
}
// When UNLOCK_BUTTON is not held, the meter is unchanged by stick input
```

## Using with a regular gamepad (no Ojo del Sol)

Players can also use this mode with a standard controller. Hold R3 (or the configured unlock button) and push the right stick to the right to set the solar level — further right means more sun. Release R3 to lock the meter at the current value. This gives players manual analog control over the sensor without needing the Ojo del Sol hardware.

## Defaults

| Setting | Default | Notes |
|---------|---------|-------|
| Axis | Right Stick X+ | Configurable to any of 8 axes via `BLE_SINGLE_ANALOG_AXIS` |
| Unlock button | R3 (0x4000) | Configurable via `BLE_METER_UNLOCK_BUTTON`; can be disabled |
