#include <Arduino.h>
#include <ArduinoJson.h>
#include <math.h>
#include <timer.h>

#define THROTTLE_INPUT_PIN A0
#define THROTTLE_OUTPUT_PIN 5
#define SENSOR_MIN 187
#define SENSOR_MAX 820

#define THROTTLE_MAX 100
int throttleMax = THROTTLE_MAX;
int throttleOverride = -1;
int currentThrottle = 0;

Timer sendTimer;


void onTimerFired() {
  // Create a comma-delimited string
  String packet = String(throttleMax) + "," + String(throttleOverride == -1 ? currentThrottle : throttleOverride);

  // Send via Serial
  Serial.println(packet);
}

// String buffer to accumulate incoming characters from Serial
String incomingData = "";

/**
 * Parse incoming data in the format:
 *   "<throttleMax>,<throttleOverride>"
 * Example: "80,50" -> throttleMax=80, throttleOverride=50
 */
void parseIncomingData(const String &data) {
  // Find the comma
  int commaIndex = data.indexOf(',');
  if (commaIndex == -1) {
    // No comma, not a valid packet
    return;
  }

  // Extract substrings
  String maxStr      = data.substring(0, commaIndex);
  String overrideStr = data.substring(commaIndex + 1);

  // Convert to int
  throttleMax     = maxStr.toInt();
  throttleOverride = overrideStr.toInt();
}

void setup() {
  Serial.begin(9600);
  pinMode(THROTTLE_INPUT_PIN, INPUT);
  pinMode(THROTTLE_OUTPUT_PIN, OUTPUT);
  sendTimer.setInterval(50);
  sendTimer.setCallback(&onTimerFired);
  sendTimer.start();
}

void loop() {
  // Read any available characters from Serial
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    // Use newline (\n) as the end-of-packet marker
    if (c == '\n') {
      // We've got a full line, parse it
      parseIncomingData(incomingData);
      incomingData = ""; // Reset buffer
    }
    else if (c != '\r') {
      // Append all other characters except carriage return (\r)
      incomingData += c;
    }
  }

  sendTimer.update();
  delay(1);

  int value = throttleOverride == -1 ? min(100, max(0, map(analogRead(THROTTLE_INPUT_PIN), SENSOR_MIN, SENSOR_MAX, 0, 100))) : throttleOverride;
  currentThrottle = value;
  int normalizedValue = min(throttleMax, max(0, map(value, 0, 100, 0, throttleMax)));
  analogWrite(THROTTLE_OUTPUT_PIN, map(normalizedValue, 0, 100, 0, 255));
}
