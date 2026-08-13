#include <Arduino.h>

#include "Nextion_Display.hpp"

// ======================================================
// TIMER UPDATE DISPLAY
// ======================================================
unsigned long tDisplay = 0;
float pesoCalibracao1 = 84000.0f; // PESO CALIBRAÇÃO

// ======================================================
// SETUP
// ======================================================
void setup()
{
    Serial.begin(115200);

    // ==================================================
    // EXEMPLO DE VALORES
    // ==================================================
    tdata = "15/05/2026";

    thora = "14:30";

    tplaca = "";

    pesoAtual = 125.4;

    ttotal = 0.0;

    ttara = 1200.0;

    // ==================================================
    // INICIA DISPLAY
    // ==================================================
    initNextion();

    Serial.println("Sistema iniciado");
}

// ======================================================
// LOOP
// ======================================================
void loop()
{
    // ==================================================
    // EXEMPLO:
    // Atualiza display a cada 2 segundos
    // ==================================================

    if (millis() - tDisplay > 500)
    {
        // Simula alteração de peso
        //pesoAtual += 1.5;

        tplaca = placaVeiculo;
        //thora = "14:30";
        //tdata = "15/05/2026";
        //ttara = ttotal;

        updateDisplay();
        tDisplay = millis();
    }  

    // ==================================================
    // PROCESSA BOTÕES / EVENTOS NEXTION
    // ==================================================
    processNextionCommands();
}