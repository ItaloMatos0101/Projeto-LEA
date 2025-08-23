#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  pinMode(48, OUTPUT);
}

void loop() {
  Serial.println("ESP32-S3 Super Mini funcionando!");
  digitalWrite(48, HIGH);
  delay(1000);
  digitalWrite(48, LOW);
  delay(1000);
}
