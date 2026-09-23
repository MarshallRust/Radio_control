# Radio Control

Arduino code for tuning an FM radio without touching it. The radio module I'm using has a rotary encoder for tuning, so instead of turning the knob, the Arduino Mega is wired into the encoder's A and B lines and fakes the pulses a real knob would make. The radio also reports its current frequency over serial, so the Mega always knows where it actually is and keeps stepping until it gets to the frequency I want.

The target frequency comes from either a potentiometer or a VL53L0X distance sensor. With the distance sensor you move your hand closer or farther away to sweep across the band (50mm is 87 MHz, 360mm is 108 MHz).

## Hardware

- Arduino Mega 2560. It has to be a Mega because I need `Serial1` for the radio and `Serial` for debugging.
- FM radio module with a rotary encoder tuner that prints `NEXT_FREQ:` / `PREV_FREQ:` over UART at 38400 baud
- 10k pot on A0, or an Adafruit VL53L0X on I2C
- LED for a heartbeat

Encoder A goes to pin 22 and B goes to pin 23. Wiring pictures are in `layout.png` and `schematic.png` in the sketch folders.

## Sketches

### Measure_light_level/Measure_light_level.ino

The pot version. The folder name is left over from the Arduino example I started from, it doesn't measure light.

- `stepCW()` / `stepCCW()` - fake one click of the encoder. Sets A then B high, then A then B low (or B first for the other direction), with 2000µs between each edge. That's the quadrature pattern the radio expects. Both update `lastMoveTime`.
- `mapFloat()` - Arduino's `map()` but for floats, since `map()` only does integers and I need 0.1 MHz steps.
- `setup()` - pins, `Serial` at 115200 for the monitor, `Serial1` at 38400 for the radio.
- `readRadioUART()` - reads characters from the radio until a newline. If the line starts with `NEXT_FREQ:` or `PREV_FREQ:` it takes the number after it and divides by 10 (the radio sends 1011 for 101.1) and saves it as `reportedFreq`.
- `loop()` -
  1. Reads the radio and the pot.
  2. Startup check: the LED stays on and nothing happens until the pot is turned all the way down. That way the radio doesn't jump across the band as soon as it powers on.
  3. Maps the pot to a frequency (flipped, so the low end of the pot is 108 and the high end is 87) and rounds to 0.1.
  4. Compares target to reported. If the difference is bigger than the deadband it steps once toward the target.
  5. The deadband is 0.15 MHz normally, and goes up to 0.5 after 2 seconds of not moving. Pots are noisy and it would twitch back and forth on a station otherwise.
  6. Prints target, reported, and deadband to the serial monitor.

### DistanceMapping/sketch_feb4a/sketch_feb4a.ino

Test sketch to make sure the distance sensor works before hooking it into the rest.

- `setup()` - starts I2C and the sensor, and stops there if it can't find the sensor.
- `loop()` - takes a reading, clamps it to 50-360mm, maps that to 87-108 MHz, and prints it. A reading with status 4 (out of range, nothing in front of it) gets treated as max distance.

### volumeControl_inputToRotaryEncoder/volumeControl_inputToRotaryEncoder.ino

The full version. Distance sensor input, encoder output, and radio feedback all together, plus a way to tell if steps are getting missed.

- `stepCW()` / `stepCCW()` - same as the pot version.
- `readRadioUART()` - same idea, but it uses a fixed `char[32]` buffer instead of an Arduino `String` so it isn't allocating memory on every character.
- `readToF()` - reads the sensor and sets `targetFreq` the same way the test sketch does.
- `analyzeStep(before, after, dir)` - after each step it checks how far the reported frequency actually moved. About 0.1 counts as OK, basically 0 is a SKIP, and more than 0.15 is a DOUBLE. It keeps running totals and prints them. It's there for tuning the pulse timing. If the pulses are too fast the radio misses them, and if the timing's off it counts one pulse twice.
- `setup()` - pins, both serial ports, I2C, sensor.
- `loop()` - reads the radio and sensor, blinks the LED, and if the target is more than 0.12 MHz away and it's been 20ms since the last step, it steps once, waits 15ms for the radio to update, reads it again, and runs `analyzeStep`. Prints target and reported every 120ms.

## Uploading

Arduino IDE, board set to Arduino Mega 2560. The distance sketches need the Adafruit_VL53L0X library from the library manager. Serial monitor at 115200.
