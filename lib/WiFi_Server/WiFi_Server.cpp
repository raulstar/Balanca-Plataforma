#include "WiFi_Server.hpp"

// Forward declaration of the tare function defined in main.cpp.
extern void tareAllSensors();
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "PaginaHTML.h"

WebServer server(80);

// Default credentials for station mode (client)
String sta_ssid = "raulstar";
String sta_password = "72989400";

// Credentials for AP mode – can be changed via setAPMode if needed
String ap_ssid = "Balanca_AP";
String ap_password = "12345678";

// Global flag indicating AP mode (default false – station mode)
bool g_apMode = false;

// Global flag indicating connection status (true when STA is connected).
// In AP mode this flag is always true because the ESP creates its own AP.
bool g_wifiConnected = false;

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
        bool result = WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
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
        // In AP mode we consider ourselves always "connected"
        g_wifiConnected = true;
    } else {
        // Station mode – connect to existing Wi‑Fi network
        WiFi.mode(WIFI_STA);
        WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
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
            g_wifiConnected = true;
        } else {
            Serial.println("\nFalha ao conectar no WiFi.");
            g_wifiConnected = false;
        }
    }
    // ======================================================
}

// ---------------------------------------------------------------------------
// monitorWiFi – called periodically to ensure the device stays connected.
// In AP mode the function does nothing because the ESP is always reachable.
// In station mode it checks the connection status and attempts a reconnection
// using the stored credentials when the link is lost.
// ---------------------------------------------------------------------------
void monitorWiFi()
{
    if (g_apMode) {
        // No monitoring needed in AP mode.
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (!g_wifiConnected) {
            Serial.println("WiFi reconectado.");
        }
        g_wifiConnected = true;
        return;
    }

    // Not connected – attempt reconnection
    Serial.println("WiFi desconectado – tentando reconectar...");
    g_wifiConnected = false;
    // Simple reconnection strategy: call WiFi.reconnect() which uses the
    // credentials supplied in the previous WiFi.begin(). If that fails, we
    // fall back to a full WiFi.begin() after a short delay.
    WiFi.reconnect();
    unsigned long start = millis();
    while (millis() - start < 5000 && WiFi.status() != WL_CONNECTED) {
        delay(200);
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Reconnection successful.");
        if (MDNS.begin("balanca")) {
            MDNS.addService("http", "tcp", 80);
        }
        g_wifiConnected = true;
    } else {
        Serial.println("Reconnection failed – will retry on next monitor call.");
    }
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

// Handler for "/zero" endpoint – performs tare (zero) of all sensors.
// Returns a simple plain‑text confirmation.
void handleZero()
{
    // Reuse existing tare wrapper to zero all sensors.
    tareAllSensors();
    server.send(200, "text/plain", "Sensors zeroed");
}

// Handler for "/dados" endpoint – returns current weight data.
// For now we provide a minimal JSON payload with the current total weight.
void handleDados()
{
    // Assuming pesoAtual is a global variable representing the current weight.
    extern float pesoAtual; // declared elsewhere (e.g., in main.cpp)
    String json = "{\"peso\":" + String(pesoAtual, 2) + "}";
    server.send(200, "application/json", json);
}