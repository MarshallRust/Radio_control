# Radio Control

Arduino sketches that tune an FM radio (87–108 MHz) **without touching its tuning knob**. An Arduino Mega pretends to be the radio's rotary encoder: it drives the encoder's A/B lines directly with quadrature pulses, reads the frequency the radio reports back over UART, and steps until the radio reaches the target frequency.

The target comes from a **potentiometer** or a **time-of-flight distance sensor**. With the distance sensor, moving your hand closer to or farther from it sweeps the dial, like a theremin for FM stations.

## How it works

```
 input (pot or VL53L0X)  ──►  target frequency (MHz)
                                   │
                                   ▼
               Arduino Mega: compare target vs. reported
                                   │ step CW / CCW
                                   ▼
         encoder A/B pins (22, 23) ──► radio tuner
                                   ▲
         Serial1 @ 38400 ◄── radio reports "NEXT_FREQ:1011" / "PREV_FREQ:…"
```

- **Closed-loop control.** It doesn't count steps blind. After each step it reads the radio's own reported frequency, so missed or doubled steps correct themselves.
- **Deadband filtering** stops it jittering back and forth around the target. The pot version widens the deadband after 2 s of no movement so a noisy pot doesn't cause drift.
- **Skip / double-step analyzer** (ToF version) logs whether each emulated encoder step moved the radio 0, 1 or 2 channels. That was used to tune the pulse timing.
- **Startup alignment** (pot version): the pot has to be turned to its minimum before control starts, so the radio doesn't jump on power-up.

## Sketches

| Folder | What it does |
|---|---|
| `Measure_light_level/` | Pot → frequency controller with deadband and hold filter. (The folder name is left over from the Arduino example it started from.) |
| `DistanceMapping/sketch_feb4a/` | Test sketch: reads the VL53L0X and prints the mapped frequency (50–360 mm → 87–108 MHz). |
| `volumeControl_inputToRotaryEncoder/` | Full version: VL53L0X distance → frequency, encoder emulation, UART feedback and the skip analyzer. |

Wiring diagrams are included as `layout.png` / `schematic.png` in the relevant folders.

## Hardware

- Arduino Mega 2560 (needs the second hardware serial port, `Serial1`)
- FM radio module with a rotary-encoder tuner and a UART frequency report
- 10k potentiometer on A0, or an Adafruit VL53L0X time-of-flight sensor over I²C
- Status LED

## Building

Open a sketch folder in the Arduino IDE, select **Arduino Mega 2560**, install the **Adafruit_VL53L0X** library for the ToF sketches, and upload. Use the Serial Monitor at 115200 baud for debug output.
