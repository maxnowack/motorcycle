#include <Arduino.h>
#include <X9C.h>
#include <math.h>

#define DEBUG 0
#define CALIBRATE_ON_START 0
#define CALIBRATION_TIME 3000
#define SENSOR_MIN 190 // sensor min value if calibration is disabled
#define SENSOR_MAX 850 // sensor max value if calibration is disabled

#define X9C_INC 4
#define X9C_UD 3
#define X9C_CS 5
#define LOCK_PIN_IN 10
#define LOCK_PIN_OUT 2
#define THROTTLE_PIN A0

#if DEBUG >= 1
  #define debugPrintLn(x)  Serial.println(x)
  #define debugPrint(x)  Serial.print(x)
#else
  #define debugPrintLn(x)
  #define debugPrint(x)
#endif

X9C pot;
int sensorMin = CALIBRATE_ON_START ? 1023 : SENSOR_MIN; // minimum sensor value
int sensorMax = CALIBRATE_ON_START ? 0 : SENSOR_MAX; // maximum sensor value
int millisStop = 0;
bool calibrated = false;

float normalizeValue(float current, float start, float stop, float startValue, float stopValue) {
  float percent = round(max(0, min(100, 100 / (stop - start) * (current - start))));
  float range = stopValue > startValue ? stopValue - startValue : startValue - stopValue;
  float value = range / 100 * percent;
  float normalizedValue = startValue + (stopValue > startValue ? value : value * -1);
  return normalizedValue;
}

void calibrate() {
  if (calibrated) return;
  calibrated = true;
  debugPrintLn("calibrating ...");
  unsigned long millisStop = millis() + CALIBRATION_TIME;
  while (millis() < millisStop) {
    debugPrint(millis());
    debugPrint(" < ");
    debugPrintLn(millisStop);
    int sensorValue = analogRead(THROTTLE_PIN);

    // record the maximum sensor value
    if (sensorValue > sensorMax) {
      sensorMax = sensorValue;
    }

    // record the minimum sensor value
    if (sensorValue < sensorMin) {
      sensorMin = sensorValue;
    }
  }
  debugPrint("sensorMin: ");
  debugPrint(sensorMin);
  debugPrint("; sensorMax: ");
  debugPrintLn(sensorMax);
}

void setPotValue() {
  int valIn = analogRead(THROTTLE_PIN);
  int valNormalized = normalizeValue(valIn, sensorMin, sensorMax, 0, 100);
  pot.setPot(valNormalized, true);
  debugPrint("in: ");
  debugPrint(valIn);
  debugPrint(" normalized: ");
  debugPrintLn(valNormalized);
}

void setup() {
  Serial.begin(9600);
  pinMode(LOCK_PIN_IN, INPUT);
  pinMode(LOCK_PIN_OUT, OUTPUT);
  pinMode(THROTTLE_PIN, INPUT);
  pinMode (X9C_CS, OUTPUT);
  pinMode (X9C_UD, OUTPUT);
  pinMode (X9C_INC, OUTPUT);

  pot.begin(X9C_CS,X9C_INC,X9C_UD);
  pot.setPotMax(true);
}

int lastState = 0;
void loop() {
  int unlocked = digitalRead(LOCK_PIN_IN);


  if (unlocked) {
    if (CALIBRATE_ON_START && unlocked != lastState) calibrate();
    setPotValue();
  } else {
    if (unlocked != lastState) {
      calibrated = false;
      pot.setPotMax(true);
    }
  }
  lastState = unlocked;
  digitalWrite(LOCK_PIN_OUT, unlocked ? 0 : 1);
}
