#include <Arduino.h>
#include "HX711_Module.hpp"

// UART do sensor
HardwareSerial SerialPort(2);

// variável global usada pela biblioteca
float valorLido = 0.0;

// objeto da balança
HX711 balanca;

// peso conhecido para calibração
float pesoCalibracao = 78000.0;
static char rxBuffer[32];
static uint8_t rxIndex = 0;

void realizarTara()
{
    // usa leitura atual
    balanca.tare(valorLido);

    Serial.println("--------------------------------");
    Serial.println("BALANCA ZERADA COM SUCESSO");
    Serial.println("--------------------------------");
}

void realizarCalibracao(float pesoConhecido)
{
    balanca.calibra(pesoConhecido);

    Serial.println("--------------------------------");
    Serial.print("BALANCA CALIBRADA COM ");
    Serial.print(pesoConhecido);
    Serial.println(" g");
    Serial.println("--------------------------------");
}

void setup()
{
    Serial.begin(115200);

    // RX = GPIO16
    // TX = GPIO17
    SerialPort.begin(9600, SERIAL_8N1, 16, 17);

    delay(1000);

    Serial.println("\n=== SISTEMA BALANCA ===");

    Serial.println("Comandos:");
    Serial.println("t -> Tara / Zerar");
    Serial.println("c -> Calibrar");
    Serial.println("========================");
}

void loop()
{
    // leitura do sensor
    while (SerialPort.available())
    {
        char c = SerialPort.read();

        // fim da linha
        if (c == '\n')
        {
            // finaliza string
            rxBuffer[rxIndex] = '\0';

            // converte para float
            valorLido = atof(rxBuffer);

            // reinicia buffer
            rxIndex = 0;

            // cálculo do peso
            float pesoGramas = balanca.get_units();

            float pesoKg = pesoGramas / 1000.0f;

            // saída
            Serial.print("Leitura Bruta: ");
            Serial.print(valorLido, 3);

            Serial.print(" | Peso: ");
            Serial.print(pesoKg, 3);

            Serial.println(" kg");
        }
        else
        {
            // evita overflow
            if (rxIndex < sizeof(rxBuffer) - 1)
            {
                rxBuffer[rxIndex++] = c;
            }
        }
    }
    // comandos do monitor serial
    if (Serial.available())
    {
        char comando = Serial.read();

        switch (comando)
        {
        case 't':
        case 'T':
            realizarTara();
            break;

        case 'c':
        case 'C':

            Serial.println("\nCOLOQUE O PESO DE CALIBRACAO...");
            delay(5000);

            realizarCalibracao(pesoCalibracao);

            break;
        }
    }

    delay(100);
}