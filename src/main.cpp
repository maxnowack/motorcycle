#include <Arduino.h>
#include <math.h>

int throttlePin = A0;
int lockPin = 2;
int isUnlocked = 0;

float normalizeValue(float current, float start, float stop, float startValue, float stopValue) {
  float percent = round(max(0, min(100, 100 / (stop - start) * (current - start))));
  float range = stopValue > startValue ? stopValue - startValue : startValue - stopValue;
  float value = range / 100 * percent;
  float normalizedValue = startValue + (stopValue > startValue ? value : value * -1);
  return normalizedValue;
}

void setup() {
  Serial.begin(9600);
  pinMode(lockPin, INPUT);
}

int calibratedStart = 0;
void loop() {
  int unlocked = digitalRead(lockPin);
  if (isUnlocked != unlocked) {
    isUnlocked = unlocked;
    if (isUnlocked) {
      for (int i = 0; i <= 50; i++) {
        int value = analogRead(throttlePin);
        delay(10);
        calibratedStart = ((calibratedStart * i) + value) / (i + 1);
      }
      Serial.println(calibratedStart);
      Serial.println("unlocked. start driving");
    } else {
      Serial.println("locked");
    }
  }
  if (isUnlocked) {
    int val = analogRead(throttlePin);
    float valF = normalizeValue(val, calibratedStart + 10, 840, 0, 255);
    Serial.println(valF);
    delay(250);
  }
}
