#include <Arduino.h>
#include "WiFi_Server.hpp"
#include "Nextion_Display.hpp"
/////////////////////////////////////////////////////////////////////////////
// VARIÁVEIS GLOBAIS

float pesoAtual = 0.0;
HardwareSerial SerialPort(2);
/////////////////////////////////////////////////////////////////////////////
// FUNÇÕES DO SISTEMA

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

/////////////////////////////////////////////////////////////////////////////
void atualizarPesoNaTela()
{
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%.2f Kg", pesoAtual);
  nextionCmd(String("tPeso.txt=\"") + buffer + "\"");
}
/////////////////////////////////////////////////////////////////////////////
// SETUP
void setup()
{
    Serial.begin(115200);
    SerialPort.begin(9600, SERIAL_8N1, 16, 17);
    initWiFi();
    initWebServer();
    initNextion();

    // 🌐 ROTAS WEB
    server.on("/zero", handleZero);
    server.on("/dados", handleDados);
    server.on("/calibrar", handleCalibrar);
}

/////////////////////////////////////////////////////////////////////////////
//  LOOP
void loop()
{
    handleWeb();

    lerNextion(); // 🔥 leitura da IHM
    atualizarPesoNaTela();
    delay(10);

    // Se houver dados no monitor serial (USB)
  while (Serial.available())
  {
    SerialPort.write(Serial.read());
  }
  static String buffer = "";

  while (SerialPort.available())
  {
    char c = SerialPort.read();

    // Se chegar fim de linha, processa
    if (c == '\n')
    {
      buffer.trim();

      if (buffer.length() > 0)
      {
        pesoAtual = buffer.toFloat();
        Serial.println("Peso recebido: " + String(pesoAtual));
      }

      buffer = ""; // limpa buffer
    }
    else
    {
      buffer += c;
    }
  }
    
}