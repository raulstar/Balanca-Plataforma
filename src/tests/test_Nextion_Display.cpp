#include <Arduino.h>

#include "Nextion_Display.hpp"

// ======================================================
// TIMER UPDATE DISPLAY
// ======================================================
unsigned long tDisplay = 0;

// ======================================================
// SETUP
// ======================================================
void setup()
{
    Serial.begin(115200);

    // ==================================================
    // EXEMPLO DE VALORES
    // ==================================================
    gdata = "15/05/2026";

    ghora = "14:30";

    gplaca = "BRA2E19";

    pesoAtual = 125.4;

    ttotal = 0.0;

    gtara = 1200.0;

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
    if (millis() - tDisplay > 2000)
    {
        // Simula alteração de peso
        pesoAtual += 1.5;

        // Atualiza display
        updateDisplay();

        Serial.println("Display atualizado");

        tDisplay = millis();
    }

    // ==================================================
    // PROCESSA BOTÕES / EVENTOS NEXTION
    // ==================================================
    processNextionCommands();
}