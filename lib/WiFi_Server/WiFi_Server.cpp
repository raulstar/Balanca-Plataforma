#include "WiFi_Server.hpp"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "PaginaHTML.h"

WebServer server(80);

// Default credentials for station mode (client)
const char *sta_ssid = "raulstar";
const char *sta_password = "72989400";

// Credentials for AP mode – can be changed via setAPMode if needed
const char *ap_ssid = "Balanca_AP";
const char *ap_password = "12345678";

// Global flag indicating AP mode (default false – station mode)
bool g_apMode = false;

void setAPMode(bool enable) {
    g_apMode = enable;
}
int tentativas = 2;
const int maxTentativas = 10;

bool ledState = false;

void handleRoot()
{
    server.send_P(200, "text/html", pagina_html);
}

void handleStatus()
{
    String json = "{";
    json += "\"wifi\":\"Conectado\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

void handleCmd()
{
    String cmd = server.arg("c");

    if (cmd == "led_on")
    {
        digitalWrite(2, HIGH);
        ledState = true;
    }
    else if (cmd == "led_off")
    {
        digitalWrite(2, LOW);
        ledState = false;
    }

    server.send(200, "text/plain", "OK");
}

void handlePeso()
{
    float peso = random(100, 500); // simulação
    server.send(200, "text/plain", String(peso));
}

void initWiFi()
{
    if (g_apMode) {
        // Start Access Point mode
        Serial.println("Iniciando modo Access Point...");
        WiFi.mode(WIFI_AP);
        bool result = WiFi.softAP(ap_ssid, ap_password);
        if (result) {
            Serial.println("AP iniciado com sucesso.");
            Serial.print("SSID: ");
            Serial.println(ap_ssid);
            Serial.print("IP: ");
            Serial.println(WiFi.softAPIP());
        } else {
            Serial.println("Falha ao iniciar AP.");
        }
        // MDNS can also be started for AP if desired
        if (MDNS.begin("balanca-ap")) {
            MDNS.addService("http", "tcp", 80);
        }
    } else {
        // Station mode – connect to existing Wi‑Fi network
        WiFi.mode(WIFI_STA);
        WiFi.begin(sta_ssid, sta_password);
        while (WiFi.status() != WL_CONNECTED && tentativas < maxTentativas) {
            delay(500);
            Serial.print('.');
            tentativas++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi Conectado!");
            Serial.println("IP: " + WiFi.localIP().toString());
            if (MDNS.begin("balanca")) {
                Serial.println("MDNS iniciado: http://balanca.local");
                MDNS.addService("http", "tcp", 80);
            } else {
                Serial.println("Erro ao iniciar MDNS");
            }
        } else {
            Serial.println("\nFalha ao conectar no WiFi.");
        }
    }
    // ======================================================
}

void initWebServer()
{

    pinMode(2, OUTPUT);

    server.on("/", handleRoot);
    server.on("/status", handleStatus);
    server.on("/cmd", handleCmd);
    server.on("/peso", handlePeso);

    server.begin();
}

void handleWeb()
{
    server.handleClient();
}