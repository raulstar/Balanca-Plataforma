#include <Arduino.h>

#define LED 2 // Pino padrão do LED onboard no ESP32

void setup() {
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, HIGH);
  delay(100); 
  digitalWrite(LED, LOW);
  delay(100);
}