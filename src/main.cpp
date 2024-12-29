#include <Arduino.h>
#include <ArduinoJson.h>
#include <math.h>
#include <timer.h>
#include "serial.h"

#define THROTTLE_INPUT_PIN A0
#define THROTTLE_OUTPUT_PIN 5
#define SENSOR_MIN 187
#define SENSOR_MAX 820

#define THROTTLE_MAX 100
int throttleMax = THROTTLE_MAX;
int throttleOverride = -1;
int currentThrottle = 0;

Timer sendTimer;

void onPacketReceived(const uint8_t* buffer, size_t size) {
  StaticJsonDocument<1024> doc;

  DeserializationError err = deserializeJson(doc, buffer, size);
  if (err) {
    // handle error
  }

  throttleMax = doc["throttleMax"];
  throttleOverride = doc["throttle"];
}

void onTimerFired() {
  StaticJsonDocument<256> doc;

  doc["throttleMax"] = throttleMax;
  doc["throttle"]    = (throttleOverride == -1) ? currentThrottle : throttleOverride;

  // // Serialize to a buffer
  uint8_t buffer[256];
  size_t usedSize = serializeJson(doc, buffer);

  // // Send the utilized portion of the buffer
  serialSend(buffer, usedSize);

  // serializeJson(doc, Serial);
  // Serial.println();
}

void setup() {
  serialBegin(9600);
  serialSetPacketHandler(&onPacketReceived);
  pinMode(THROTTLE_INPUT_PIN, INPUT);
  pinMode(THROTTLE_OUTPUT_PIN, OUTPUT);
  sendTimer.setInterval(100);
  sendTimer.setCallback(&onTimerFired);
  sendTimer.start();
}

void loop() {
  serialUpdate();
  sendTimer.update();
  delay(1);

  int value = throttleOverride == -1 ? min(100, max(0, map(analogRead(THROTTLE_INPUT_PIN), SENSOR_MIN, SENSOR_MAX, 0, 100))) : throttleOverride;
  currentThrottle = value;
  int normalizedValue = min(throttleMax, max(0, map(value, 0, 100, 0, throttleMax)));
  analogWrite(THROTTLE_OUTPUT_PIN, map(normalizedValue, 0, 100, 0, 255));
}
