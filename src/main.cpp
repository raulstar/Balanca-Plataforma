#include <Arduino.h>

#include "WiFi_Server.hpp"
#include "Nextion_Display.hpp"
#include "HX711_Module.hpp"
#include "Thermal_Printer.hpp"

#define M0 19
#define M1 21
#define OK 32

#define ok digitalRead(OK)
#define m1 digitalWrite(M1, LOW)
#define m0 digitalWrite(M0, LOW)
/////////////////////////////////////////////////////////////////////////////
// VARIÁVEIS GLOBAIS

unsigned long tDisplay = 0;
HardwareSerial SerialPort(3);
HardwareSerial impressoraSerial(2);
float pesoCalibracao1 = 78000.0f;
bool imprimir;

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

void printTask(void *pvParameters) {
    DadosImpressao *dados = (DadosImpressao *)pvParameters;
    imprimirRegistro(*dados);
    delete dados;
    vTaskDelete(NULL);
}

void processarImpressao() {
  if (imprimir) {
    DadosImpressao *dados = new DadosImpressao;
    dados->placa = placaVeiculo;
    dados->data = tdata;
    dados->hora = thora;
    dados->contador = contadorRegistro;
    dados->total = ttotal;
    dados->tara = ttara;
    dados->eixo = eixo;
    dados->peixo1 = peixo1;
    dados->peixo2 = peixo2;
    dados->peixo3 = peixo3;
    dados->peixo4 = peixo4;
    dados->peixo5 = peixo5;
    dados->peixo6 = peixo6;

    xTaskCreate(printTask, "PrintTask", 4096, dados, 1, NULL);
    imprimir = false;
    Serial.println("Registro sendo impresso...");
  }
}

void processarSerial() {
  while (Serial.available()) {
    //////////////////////////////////////////////////////////////////////////
    // COMANDOS
    //////////////////////////////////////////////////////////////////////////
    if (calibrando1) {
      Serial.print("calibrando1 ");
      Serial.println(pesoCalibracao1);
      sensor1.calibra(pesoCalibracao1);
      calibrando1 = false;
    }
    if (Serial.available()) {
      char comando = Serial.read();

      switch (comando) {
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
  iniciarImpressora(impressoraSerial, 9600, 4, 5);

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
  processarImpressao();

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
  if (zero)
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

  processarSerial();

  delay(5);
}