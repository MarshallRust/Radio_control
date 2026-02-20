// === Pin definitions ===
#define POT_PIN A0
#define ENC_A 22
#define ENC_B 23
#define LED 13

// === Timing ===
#define STEP_DELAY 2000  // microseconds between encoder edges

// === Frequency range ===
#define FREQ_MIN 87.0
#define FREQ_MAX 108.0
#define STEP_SIZE 0.1  // MHz per encoder step

// === Deadband settings ===
const float NORMAL_DEAD_BAND = 0.15; // small flicker filter
const float LONG_DEAD_BAND   = 0.5;  // bigger threshold if idle too long
const unsigned long HOLD_TIME = 2000; // ms before long deadband

// === State ===
bool canStart = false;
float reportedFreq = FREQ_MIN;   // comes from UART
float targetFreq   = FREQ_MIN;
unsigned long lastMoveTime = 0;

String uartLine = "";

// === Step encoder functions ===
void stepCW() {
  digitalWrite(ENC_A, HIGH);
  delayMicroseconds(STEP_DELAY);
  digitalWrite(ENC_B, HIGH);
  delayMicroseconds(STEP_DELAY);
  digitalWrite(ENC_A, LOW);
  delayMicroseconds(STEP_DELAY);
  digitalWrite(ENC_B, LOW);
  delayMicroseconds(STEP_DELAY);
  lastMoveTime = millis();
}

void stepCCW() {
  digitalWrite(ENC_B, HIGH);
  delayMicroseconds(STEP_DELAY);
  digitalWrite(ENC_A, HIGH);
  delayMicroseconds(STEP_DELAY);
  digitalWrite(ENC_B, LOW);
  delayMicroseconds(STEP_DELAY);
  digitalWrite(ENC_A, LOW);
  delayMicroseconds(STEP_DELAY);
  lastMoveTime = millis();
}

// === Float mapping function ===
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// === Setup ===
void setup() {
  pinMode(ENC_A, OUTPUT);
  pinMode(ENC_B, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(ENC_A, LOW);
  digitalWrite(ENC_B, LOW);
  digitalWrite(LED, LOW);

  Serial.begin(115200);     // USB monitor
  Serial1.begin(38400);     // radio UART
}

// === UART parser ===
void readRadioUART() {
  while (Serial1.available()) {
    char c = Serial1.read();

    if (c == '\n') {
      if (uartLine.startsWith("NEXT_FREQ:") || uartLine.startsWith("PREV_FREQ:")) {
        int raw = uartLine.substring(10).toInt();
        reportedFreq = raw / 10.0;  // convert to MHz
      }
      uartLine = "";
    } else if (c != '\r') {
      uartLine += c;
    }
  }
}

// === Main loop ===
void loop() {
  readRadioUART();

  int potValue = analogRead(POT_PIN);

  // === STARTUP ALIGNMENT ===
  if (!canStart) {
    digitalWrite(LED, HIGH);
    if (potValue < 5) {
      canStart = true;
      delay(300);
    }
    return;
  }

  // === HEARTBEAT ===
  digitalWrite(LED, millis() % 400 < 40);

  // === Pot → target frequency (flipped) ===
  targetFreq = mapFloat(potValue, 0, 1023, FREQ_MAX, FREQ_MIN);
  targetFreq = round(targetFreq * 10.0) / 10.0;

  // === Step toward target using serial-reported frequency with flicker + long hold filter ===
  float diff = targetFreq - reportedFreq;

  unsigned long idleTime = millis() - lastMoveTime;
  float deadband = (idleTime > HOLD_TIME) ? LONG_DEAD_BAND : NORMAL_DEAD_BAND;

  if (diff > deadband) {
    stepCW();
  } 
  else if (diff < -deadband) {
    stepCCW();
  }

  // === Debug output ===
  Serial.print("Pot Target: ");
  Serial.print(targetFreq);
  Serial.print("  Reported: ");
  Serial.print(reportedFreq);
  Serial.print("  Deadband: ");
  Serial.println(deadband);
}
