#include <Arduino.h>

#include "WiFi_Server.hpp"
#include "Nextion_Display.hpp"
#include "HX711_Module.hpp"

#define M0 19
#define M1 21
#define OK 32

#define ok digitalRead(OK)
#define m1 digitalWrite(M1, LOW)
#define m0 digitalWrite(M0, LOW)
/////////////////////////////////////////////////////////////////////////////
// VARIÁVEIS GLOBAIS

unsigned long tDisplay = 0;
HardwareSerial SerialPort(2);
float pesoCalibracao1 = 78000.0f;

/////////////////////////////////////////////////////////////////////////////

SensorBalanca sensor1(SerialPort); // INSTÂNCIA SENSOR

void handleZero() // FUNÇÕES WEB
{
  pesoAtual = 0.0f;

  sensor1.tare();

  if (server.client())
  {
    server.send(200, "text/plain", "Zerado!");
  }

  // nextionCmd("tPeso.txt=\"Zerado!\"");
  handle_bsom();

  Serial.println("Balança zerada.");

  delay(800);
}
void handleDados() // DADOS WEB
{
  String json = "{";

  json += "\"pesoAtual\":" + String(pesoAtual, 3);

  // json += ",";

  // json += "\"noise\":" + String(sensor1.getNoise(), 5);

  // json += ",";

  // json += "\"stable\":";

  // json += sensor1.isStable() ? "true" : "false";

  json += "}";

  server.send(200, "application/json", json);
}
void handleCalibrar() // CALIBRAÇÃO WEB
{
  server.send(200, "text/plain", "Modo calibração iniciado");
}

/////////////////////////////////////////////////////////////////////////////
// SETUP
void setup()
{
  pinMode(OK, INPUT);
  pinMode(M1, OUTPUT);
  pinMode(M0, OUTPUT);
  m1;
  m0;
  Serial.begin(115200);

  SerialPort.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("\n=== SISTEMA BALANCA ===");

  Serial.println("Comandos:");
  Serial.println("t -> Tara");
  Serial.println("c -> Calibrar");

  Serial.println("========================");

  initWiFi();
  initWebServer();
  initNextion();
  sensor1.tare();

  server.on("/zero", handleZero);
  server.on("/dados", handleDados);
  server.on("/calibrar", handleCalibrar);
  Serial.println("\n=== SISTEMA BALANCA ===");

  Serial.println("Comandos:");
  Serial.println("t -> Tara");
  Serial.println("c5000 -> Calibrar com 5000g");
  Serial.println("========================");
}

/////////////////////////////////////////////////////////////////////////////
// LOOP
void loop()
{
  handleWeb();
  processNextionCommands();
  pesoAtual = sensor1.getKg();

 if (millis() - tDisplay > 2000)
    {               
        tplaca = placaVeiculo;
        thora = "14:30";
        tdata = "15/05/2026";
        ttara = ttotal;
        updateDisplay();
        tDisplay = millis();
    }


  if (calibrando1)
  {
    Serial.print("calibrando1 ");
    Serial.println(pesoCalibracao1);
    sensor1.calibra(pesoCalibracao1);
    calibrando1 = false;
  }
  if(zero)
  {
    sensor1.tare();
    zero = false;
    Serial.println("Balança zerada.");
  }
  if (sensor1.leitura())
  {
    // sensor1.getKg();

    //////////////////////////////////////////////////////////////////////////
    // DEBUG
    //////////////////////////////////////////////////////////////////////////

    Serial.print("RAW: ");

    Serial.print(sensor1.getRaw(), 3);

    Serial.print(" | KG: ");

    Serial.print(sensor1.getKg(), 3);

    Serial.println();
  }

  while (Serial.available())
  {
    //////////////////////////////////////////////////////////////////////////
    // COMANDOS
    //////////////////////////////////////////////////////////////////////////
    if (calibrando1)
    {
      Serial.print("calibrando1 ");
      Serial.println(pesoCalibracao1);
      sensor1.calibra(pesoCalibracao1);
      calibrando1 = false;
    }
    if (Serial.available())
    {
      char comando = Serial.read();

      switch (comando)
      {
        //////////////////////////////////////////////////////////////////////////
        // TARA
        //////////////////////////////////////////////////////////////////////////

      case 't':
      case 'T':
        sensor1.tare();
        break;

        //////////////////////////////////////////////////////////////////////////
        // CALIBRAÇÃO
        //////////////////////////////////////////////////////////////////////////

      case 'c':
      case 'C':

        Serial.println("\nCOLOQUE O PESO DE CALIBRACAO...");
        delay(2000);

        sensor1.calibra(pesoCalibracao1);

        break;
      }
    }
  }

  delay(5);
}