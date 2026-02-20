#include <Wire.h>
#include <Adafruit_VL53L0X.h>

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

#define DIST_OFF 50     // mm → min distance
#define DIST_ON 360     // mm → max distance
#define FREQ_MIN 87.0
#define FREQ_MAX 108.0

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!lox.begin()) {
    Serial.println("VL53L0X not found!");
    while (1);
  }

  Serial.println("VL53L0X ready!");
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  int distance;

  // If reading invalid, treat as max distance → 108 MHz
  if (measure.RangeStatus == 4) {
    distance = DIST_ON;  // max distance
  } else {
    distance = measure.RangeMilliMeter;
  }

  distance = constrain(distance, DIST_OFF, DIST_ON);

  // Map distance 50–360 → 87–108 MHz
  float freq = FREQ_MIN + (distance - DIST_OFF) * (FREQ_MAX - FREQ_MIN) / (DIST_ON - DIST_OFF);

  Serial.println(freq); // send mapped frequency to Mega

  delay(20);
}
