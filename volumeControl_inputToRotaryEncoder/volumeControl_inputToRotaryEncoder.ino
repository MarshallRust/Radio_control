#include <Wire.h>
#include <Adafruit_VL53L0X.h>

Adafruit_VL53L0X lox;

// ================= PIN DEFINITIONS =================
#define ENC_A 22
#define ENC_B 23
#define LED 2   // moved to pin 2

// ================= RADIO CONFIG =================
#define FREQ_MIN 86.9
#define FREQ_MAX 108.1
#define STEP_SIZE 0.1

// ================= TOF CONFIG =================
#define DIST_OFF 50
#define DIST_ON 360

// ================= TIMING =================
#define ENC_PULSE_US 2000
#define DEBUG_INTERVAL 120

// ================= CONTROL PARAMS =================
const float DEAD_BAND = 0.12;

// ================= STATE =================
float reportedFreq = FREQ_MIN;
float targetFreq   = FREQ_MIN;

unsigned long lastStepTime  = 0;
unsigned long lastDebugTime = 0;

// === skip detection state ===
float lastReportedFreq = FREQ_MIN;
long stepCounter = 0;
long skipCount = 0;
long doubleCount = 0;
long okCount = 0;

// UART buffer for radio feedback
char uartLine[32];
int uartIndex = 0;

// ================= ENCODER STEP =================
void stepCW() {
  digitalWrite(ENC_A, HIGH);
  delayMicroseconds(ENC_PULSE_US);
  digitalWrite(ENC_B, HIGH);
  delayMicroseconds(ENC_PULSE_US);
  digitalWrite(ENC_A, LOW);
  delayMicroseconds(ENC_PULSE_US);
  digitalWrite(ENC_B, LOW);
  delayMicroseconds(ENC_PULSE_US);
}

void stepCCW() {
  digitalWrite(ENC_B, HIGH);
  delayMicroseconds(ENC_PULSE_US);
  digitalWrite(ENC_A, HIGH);
  delayMicroseconds(ENC_PULSE_US);
  digitalWrite(ENC_B, LOW);
  delayMicroseconds(ENC_PULSE_US);
  digitalWrite(ENC_A, LOW);
  delayMicroseconds(ENC_PULSE_US);
}

// ================= RADIO UART PARSER =================
void readRadioUART() {

  while (Serial1.available()) {
    char c = Serial1.read();

    if (c == '\n') {
      uartLine[uartIndex] = '\0';

      if (strncmp(uartLine, "NEXT_FREQ:", 10) == 0 ||
          strncmp(uartLine, "PREV_FREQ:", 10) == 0) {
        int raw = atoi(uartLine + 10);
        reportedFreq = raw / 10.0;
      }

      uartIndex = 0;
    }
    else if (c != '\r') {
      if (uartIndex < sizeof(uartLine) - 1) {
        uartLine[uartIndex++] = c;
      }
    }
  }
}

// ================= TOF → TARGET FREQUENCY =================
void readToF() {

  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  int distance;

  if (measure.RangeStatus == 4) {
    distance = DIST_ON;  // invalid → max frequency
  } else {
    distance = measure.RangeMilliMeter;
  }

  distance = constrain(distance, DIST_OFF, DIST_ON);

  float freq = FREQ_MIN +
               (distance - DIST_OFF) *
               (FREQ_MAX - FREQ_MIN) /
               (DIST_ON - DIST_OFF);

  targetFreq = round(freq * 10.0) / 10.0;
}

// ================= SKIP ANALYZER =================
void analyzeStep(float before, float after, int dir) {

  float diff = after - before;
  float expected = dir * STEP_SIZE;

  stepCounter++;

  if (fabs(diff) < STEP_SIZE * 0.5) {
    skipCount++;
    Serial.print("❌ SKIP ");
  }
  else if (fabs(diff) > STEP_SIZE * 1.5) {
    doubleCount++;
    Serial.print("⚠ DOUBLE ");
  }
  else {
    okCount++;
    Serial.print("OK ");
  }

  Serial.print("step=");
  Serial.print(stepCounter);
  Serial.print(" expected=");
  Serial.print(expected, 2);
  Serial.print(" actual=");
  Serial.print(diff, 2);
  Serial.print(" freq=");
  Serial.print(after, 2);
  Serial.print(" | OK=");
  Serial.print(okCount);
  Serial.print(" SKIP=");
  Serial.print(skipCount);
  Serial.print(" DOUBLE=");
  Serial.println(doubleCount);
}

// ================= SETUP =================
void setup() {

  pinMode(ENC_A, OUTPUT);
  pinMode(ENC_B, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(ENC_A, LOW);
  digitalWrite(ENC_B, LOW);

  Serial.begin(115200);   // debug
  Serial1.begin(38400);   // radio module

  Wire.begin();

  if (!lox.begin()) {
    Serial.println("VL53L0X not found!");
    while (1);
  }

  Serial.println("=== RADIO SWEEP + SKIP DETECTOR ===");
}

// ================= MAIN LOOP =================
void loop() {

  readRadioUART();   // keep radio feedback
  readToF();         // new TOF input

  digitalWrite(LED, millis() % 500 < 40);

  float diff = targetFreq - reportedFreq;

  if (fabs(diff) > DEAD_BAND && millis() - lastStepTime > 20) {

    lastStepTime = millis();
    float before = reportedFreq;

    if (diff > 0) {
      stepCW();
    } else {
      stepCCW();
    }

    delay(15);          // allow radio to update
    readRadioUART();    // refresh reportedFreq

    float after = reportedFreq;
    analyzeStep(before, after, diff > 0 ? +1 : -1);

    lastReportedFreq = after;
  }

  if (millis() - lastDebugTime > DEBUG_INTERVAL) {
    lastDebugTime = millis();
    Serial.print("Target=");
    Serial.print(targetFreq);
    Serial.print(" Reported=");
    Serial.println(reportedFreq);
  }
}
