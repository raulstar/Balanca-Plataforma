#include "WiFi_Server.hpp"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "PaginaHTML.h"

WebServer server(80);

const char *ssid = "raulstar";
const char *password = "72989400";
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
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED && tentativas < maxTentativas)
    {
        delay(500);
        Serial.print(".");
        tentativas++;
    }

   if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi Conectado!");
    Serial.println("IP: " + WiFi.localIP().toString());

    if (MDNS.begin("balanca"))
    {
      Serial.println("MDNS iniciado: http://balanca.local");
      MDNS.addService("http", "tcp", 80);
    }
    else
    {
      Serial.println("Erro ao iniciar MDNS");
    }
  }
  else
  {
    Serial.println("\nFalha ao conectar no WiFi.");
    // Opcional: reiniciar automaticamente
    // ESP.restart();
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