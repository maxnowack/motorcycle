#include <Arduino.h>
#include <math.h>

#define DEBUG 1
#define THROTTLE_INPUT_PIN A0
#define THROTTLE_OUTPUT_PIN 5
#define SENSOR_MIN 187
#define SENSOR_MAX 820

#define THROTTLE_MIN 0
#define THROTTLE_MAX 255

#if DEBUG >= 1
  #define debugPrintLn(x)  Serial.println(x)
  #define debugPrint(x)  Serial.print(x)
#else
  #define debugPrintLn(x)
  #define debugPrint(x)
#endif

void setup() {
  Serial.begin(9600);
  pinMode(THROTTLE_INPUT_PIN, INPUT);
  pinMode(THROTTLE_OUTPUT_PIN, OUTPUT);
}

void loop() {
  delay(100);
  int value = analogRead(THROTTLE_INPUT_PIN);
  int normalizedValue = min(THROTTLE_MAX, max(THROTTLE_MIN, map(value, SENSOR_MIN, SENSOR_MAX, THROTTLE_MIN, THROTTLE_MAX)));
  debugPrintLn("value: " + String(value) + " normalized: " + String(normalizedValue));
  analogWrite(THROTTLE_OUTPUT_PIN, normalizedValue);
}
