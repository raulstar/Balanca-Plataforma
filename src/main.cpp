#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "WiFi_Server.hpp"
#include "Nextion_Display.hpp"
#include "HX711_Module.hpp"
#include "Thermal_Printer.hpp"
#include "config.hpp"

#define M0 19
#define M1 21
#define OK 32

#define ok digitalRead(OK)
#define m1 digitalWrite(M1, LOW)
#define m0 digitalWrite(M0, LOW)
/////////////////////////////////////////////////////////////////////////////
// VARIÁVEIS GLOBAIS

uint32_t displayUpdateInterval = 5000;
HardwareSerial SerialPort(2);
HardwareSerial impressoraSerial(3);
float pesoCalibracao1 = 78000.0f;
bool imprimir;

void taskUpdateDisplay(void *pvParameters)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(displayUpdateInterval);
  ESP_LOGI("Task", "Display update task started with interval %d ms", displayUpdateInterval);
  for (;;)
  {
    tplaca = placaVeiculo;
    thora = "14:30";
    tdata = "15/05/2026";
    ttara = ttotal;
    updateDisplay();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

/////////////////////////////////////////////////////////////////////////////

SensorBalanca sensor1(SerialPort, "S1");
SensorBalanca sensor2(SerialPort, "S2");
SensorBalanca sensor3(SerialPort, "S3");
SensorBalanca sensor4(SerialPort, "S4");

struct SensorConfig
{
  SensorBalanca *sensor;
  String prefixo;
};

SensorConfig sensores[] = {
    {&sensor1, "S1"},
    {&sensor2, "S2"},
    {&sensor3, "S3"},
    {&sensor4, "S4"}};
int numSensores = sizeof(sensores) / sizeof(sensores[0]);

void tareAllSensors()
{
  for (int i = 0; i < numSensores; i++)
  {
    sensores[i].sensor->tare();
  }
  Serial.println("Todas as balanças zeradas.");
}

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

void printTask(void *pvParameters)
{
  DadosImpressao *dados = (DadosImpressao *)pvParameters;
  imprimirRegistro(*dados);
  delete dados;
  vTaskDelete(NULL);
}

void processarImpressao()
{
  if (imprimir)
  {
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

void processarSerial()
{

  if (Serial.available())
  {
    String comando = Serial.readStringUntil('\n');

    comando.trim();

    //////////////////////////////////////////////////////////////////////////
    // TARA (t1-t4)
    //////////////////////////////////////////////////////////////////////////

    if (comando.startsWith("t") || comando.startsWith("T"))
    {
      int sensorIndex = comando.substring(1).toInt() - 1;
      if (sensorIndex == -1) // Tare all (e.g. "t" or "t0")
      {
        tareAllSensors();
      }
      else if (sensorIndex >= 0 && sensorIndex < numSensores)
      {
        sensores[sensorIndex].sensor->tare();
        Serial.print("Sensor ");
        Serial.print(sensorIndex + 1);
        Serial.println(" zerado.");
      }
      else
      {
        Serial.println("ERRO: Sensor invalido (use t0 para todos ou t1-t4)");
      }
    }

    //////////////////////////////////////////////////////////////////////////
    // FATOR DE ESCALA (f[n] [fator] ou g[n])
    //////////////////////////////////////////////////////////////////////////

    else if (comando.startsWith("f") || comando.startsWith("F"))
    {
      // Formato "f[n] [fator]"
      int espacoIndex = comando.indexOf(' ');
      if (espacoIndex != -1)
      {
        int sensorIndex = comando.substring(1, espacoIndex).toInt() - 1;
        float novoFator = comando.substring(espacoIndex + 1).toFloat();

        if (sensorIndex >= 0 && sensorIndex < numSensores)
        {
          sensores[sensorIndex].sensor->setScale(novoFator);
          Serial.print("Sensor ");
          Serial.print(sensorIndex + 1);
          Serial.print(" fator definido: ");
          Serial.println(novoFator, 8);
        }
        else
        {
          Serial.println("ERRO: Sensor invalido (use f[n] [fator])");
        }
      }
    }
    else if (comando.startsWith("g") || comando.startsWith("G"))
    {
      int sensorIndex = comando.substring(1).toInt() - 1;
      if (sensorIndex >= 0 && sensorIndex < numSensores)
      {
        Serial.print("Sensor ");
        Serial.print(sensorIndex + 1);
        Serial.print(" fator: ");
        Serial.println(sensores[sensorIndex].sensor->getScale(), 8);
      }
      else
      {
        Serial.println("ERRO: Sensor invalido (use g[n])");
      }
    }

    //////////////////////////////////////////////////////////////////////////
    // CALIBRAÇÃO COM PESO PADRÃO (c1, c2, c3, c4)
    //////////////////////////////////////////////////////////////////////////

    else if (comando.length() == 2 && (comando.startsWith("c") || comando.startsWith("C")))
    {
      int sensorIndex = comando.substring(1).toInt() - 1;
      if (sensorIndex >= 0 && sensorIndex < numSensores)
      {
        Serial.println("--------------------------------");
        Serial.print("COLOQUE O PESO DE CALIBRACAO NO SENSOR ");
        Serial.println(sensorIndex + 1);
        Serial.println("--------------------------------");

        delay(3000);

        sensores[sensorIndex].sensor->calibra(pesoCalibracao1);
      }
      else
      {
        Serial.println("ERRO: Sensor invalido (use c1-c4)");
      }
    }

    //////////////////////////////////////////////////////////////////////////
    // CALIBRAÇÃO COM PESO INFORMADO (c1 5000, c2 5000, ...)
    //////////////////////////////////////////////////////////////////////////

    else if (comando.startsWith("c") || comando.startsWith("C"))
    {
      // Espera formato "c[n] [peso]"
      int espacoIndex = comando.indexOf(' ');
      if (espacoIndex != -1)
      {
        int sensorIndex = comando.substring(1, espacoIndex).toInt() - 1;
        float pesoConhecido = comando.substring(espacoIndex + 1).toFloat();

        if (sensorIndex >= 0 && sensorIndex < numSensores && pesoConhecido > 0)
        {
          Serial.println("--------------------------------");
          Serial.print("AGUARDE ESTABILIZAR SENSOR ");
          Serial.println(sensorIndex + 1);
          Serial.println("--------------------------------");

          delay(3000);

          sensores[sensorIndex].sensor->calibra(pesoConhecido);
        }
        else
        {
          Serial.println("ERRO: Formato invalido ou peso/sensor invalido (use c[n] [peso])");
        }
      }
      else
      {
        Serial.println("ERRO: Formato invalido (use c[n] [peso])");
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
  xTaskCreate(taskUpdateDisplay, "UpdateDisplay", 4096, NULL, 1, NULL);
  sensor1.tare();

  server.on("/zero", handleZero);
  server.on("/dados", handleDados);
  server.on("/calibrar", handleCalibrar);
  Serial.println("\n=== SISTEMA BALANCA ===");

  Serial.println("Comandos:");
  Serial.println("t -> Tara");
  // Serial.println("c5000 -> Calibrar com 5000g");
  Serial.println("========================");
}

/////////////////////////////////////////////////////////////////////////////
// LOOP
void loop()
{
  handleWeb();
  processNextionCommands();
  pesoAtual = 0.0f;
  for (int i = 0; i < numSensores; i++)
  {
    if (sensores[i].sensor->isReady())
    {
      pesoAtual += sensores[i].sensor->getKg();
    }
  }
  processarImpressao();

  if (calibrando1)
  {
    Serial.print("calibrando1 ");
    Serial.println(pesoCalibracao1);
    sensor1.calibra(pesoCalibracao1);
    calibrando1 = false;
  }
  if (zero)
  {
    tareAllSensors();
    zero = false;
    Serial.println("Balança zerada.");
  }
   //////////////////////////////////////////////////////////////////////////
    // LEITURA SENSOR
    //////////////////////////////////////////////////////////////////////////

    static String bufferSerial;
   while (SerialPort.available())
    {
        char c = SerialPort.read();
        if (c == '\n')
        {
            for (int i = 0; i < numSensores; i++)
            {
                if (sensores[i].sensor->processaString(bufferSerial))
                {
                    Serial.print(sensores[i].prefixo + " RAW: ");
                    Serial.print(sensores[i].sensor->getRaw(), 3);

                    Serial.print(" | " + sensores[i].prefixo + " KG: ");
                    Serial.print(sensores[i].sensor->getKg(), 3);
                    Serial.print(" | Peso Atual: ");
                    Serial.println(pesoAtual, 3);
                    Serial.println();
                }
            }
            bufferSerial = "";
        }
        else if (c != '\r')
        {
            bufferSerial += c;
        }
    }

  processarSerial();

  delay(5);
}