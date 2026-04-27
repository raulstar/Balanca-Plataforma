#include <Arduino.h>
#include "WiFi_Server.hpp"

void setup() {
    Serial.begin(115200);

    initWiFi();
    initWebServer();
}

void loop() {
    handleWeb();
}