#include <Arduino.h>
#include "WiFi_Server.hpp"
#include "Nextion_Display.hpp"

// ==========================
// 🔴 VARIÁVEIS GLOBAIS
// ==========================

float pesoAtual = 0.0;

// ==========================
// 🔴 FUNÇÕES DO SISTEMA
// ==========================

void handleZero()
{
    pesoAtual = 0.0;

    if (server.client())
        server.send(200, "text/plain", "Zerado!");

    nextionCmd("tPeso.txt=\"Zerado!\"");

    Serial.println("Balança zerada.");
    delay(800);
}

// 🔹 rota web (/dados)
void handleDados()
{
    String json = "{";
    json += "\"pesoAtual\":" + String(pesoAtual, 2);
    json += "}";

    server.send(200, "application/json", json);
}

// 🔹 calibração
void handleCalibrar()
{
    server.send(200, "text/plain", "Modo calibração iniciado");
}

// ==========================
// SETUP
// ==========================

void setup()
{
    Serial.begin(115200);

    initWiFi();
    initWebServer();
    initNextion();

    // 🌐 ROTAS WEB
    server.on("/zero", handleZero);
    server.on("/dados", handleDados);
    server.on("/calibrar", handleCalibrar);
}

// ==========================
// LOOP
// ==========================

void loop()
{
    handleWeb();

    lerNextion();   // 🔥 leitura da IHM

    // 🔴 depois você coloca o HX711 aqui
    // pesoAtual = getPeso();

    delay(10);
}