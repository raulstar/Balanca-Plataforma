#include "WiFi_Server.hpp"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "PaginaHTML.h"

WebServer server(80);

const char* ssid = "Revlo_Claro";
const char* password = "Revlo@2025";

bool ledState = false;

void handleRoot() {
    server.send_P(200, "text/html", MAIN_page);
}

void handleStatus() {
    String json = "{";
    json += "\"wifi\":\"Conectado\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

void handleCmd() {
    String cmd = server.arg("c");

    if (cmd == "led_on") {
        digitalWrite(2, HIGH);
        ledState = true;
    }
    else if (cmd == "led_off") {
        digitalWrite(2, LOW);
        ledState = false;
    }

    server.send(200, "text/plain", "OK");
}

void handlePeso() {
    float peso = random(100, 500); // simulação
    server.send(200, "text/plain", String(peso));
}

void initWiFi() {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    Serial.println("WiFi conectado");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp32")) {
        Serial.println("mDNS iniciado: http://esp32.local");
    }
}

void initWebServer() {

    pinMode(2, OUTPUT);

    server.on("/", handleRoot);
    server.on("/status", handleStatus);
    server.on("/cmd", handleCmd);
    server.on("/peso", handlePeso);

    server.begin();
}

void handleWeb() {
    server.handleClient();
}