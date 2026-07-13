#include <Bluepad32.h>

// ---- MX1515 wiring (one-row layout) ----
const int VCC_PIN = 25;   // MX1515 VCC1 + VCC2 (pins 1 & 5) — arm/disarm
const int M1_INA  = 27;   // pin 2 (INA1)
const int M1_INB  = 26;   // pin 3 (INB1)
const int M2_INA  = 33;   // pin 6 (INA2)
const int M2_INB  = 32;   // pin 7 (INB2)

const int CH_M1A = 0;
const int CH_M1B = 1;
const int CH_M2A = 2;
const int CH_M2B = 3;

const int DEADZONE  = 40;
const int SLEW_STEP = 15;

ControllerPtr myController = nullptr;
int m1Current = 0;
int m2Current = 0;

void stopMotors() {
  m1Current = 0;
  m2Current = 0;
  ledcWrite(CH_M1A, 0);
  ledcWrite(CH_M1B, 0);
  ledcWrite(CH_M2A, 0);
  ledcWrite(CH_M2B, 0);
}

void writeMotor(int chA, int chB, int speed) {
  if (speed > 0)      { ledcWrite(chA, speed);  ledcWrite(chB, 0); }
  else if (speed < 0) { ledcWrite(chA, 0);      ledcWrite(chB, -speed); }
  else                { ledcWrite(chA, 0);      ledcWrite(chB, 0); }
}

int slew(int current, int target) {
  if (target == 0) return 0;
  if (target > current) return min(current + SLEW_STEP, target);
  if (target < current) return max(current - SLEW_STEP, target);
  return current;
}

int stickToSpeed(int axis) {
  if (abs(axis) < DEADZONE) return 0;
  return constrain(map(axis, -512, 511, 255, -255), -255, 255);
}

void onConnect(ControllerPtr ctl) {
  if (myController == nullptr) {
    myController = ctl;
    stopMotors();
    digitalWrite(VCC_PIN, HIGH);
    Serial.println("Controller connected");
  }
}

void onDisconnect(ControllerPtr ctl) {
  if (myController == ctl) {
    myController = nullptr;
    stopMotors();
    digitalWrite(VCC_PIN, LOW);
    Serial.println("Controller disconnected");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(VCC_PIN, OUTPUT);
  digitalWrite(VCC_PIN, LOW);

  ledcSetup(CH_M1A, 1000, 8);  ledcAttachPin(M1_INA, CH_M1A);
  ledcSetup(CH_M1B, 1000, 8);  ledcAttachPin(M1_INB, CH_M1B);
  ledcSetup(CH_M2A, 1000, 8);  ledcAttachPin(M2_INA, CH_M2A);
  ledcSetup(CH_M2B, 1000, 8);  ledcAttachPin(M2_INB, CH_M2B);
  stopMotors();

  BP32.setup(&onConnect, &onDisconnect);
  BP32.forgetBluetoothKeys();
}

void loop() {
  BP32.update();

  if (myController && myController->isConnected()) {
    int m1Target = stickToSpeed(myController->axisRY());  
    int m2Target = stickToSpeed(myController->axisY());   

    m1Current = slew(m1Current, m1Target);
    m2Current = slew(m2Current, m2Target);

    writeMotor(CH_M1A, CH_M1B, m1Current);
    writeMotor(CH_M2A, CH_M2B, m2Current);
  } else {
    stopMotors();
  }

  delay(10);
}