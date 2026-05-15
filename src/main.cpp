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

extern float pesoAtual;

HardwareSerial SerialPort(2);
float pesoCalibracao = 78000.0f;
/////////////////////////////////////////////////////////////////////////////

class SensorBalanca // CLASSE SENSOR BALANÇA
{
private:
  //////////////////////////////////////////////////////////////////////////
  // SERIAL
  //////////////////////////////////////////////////////////////////////////

  HardwareSerial *serial;

  //////////////////////////////////////////////////////////////////////////
  // BALANÇA
  //////////////////////////////////////////////////////////////////////////

  HX711 balanca;

  //////////////////////////////////////////////////////////////////////////
  // BUFFER RX
  //////////////////////////////////////////////////////////////////////////

  char rxBuffer[32];

  uint8_t rxIndex = 0;

  //////////////////////////////////////////////////////////////////////////
  // DADOS
  //////////////////////////////////////////////////////////////////////////

  float valorLido = 0.0f;

  float pesoGramas = 0.0f;

  float pesoKg = 0.0f;

  //////////////////////////////////////////////////////////////////////////
  // STATUS
  //////////////////////////////////////////////////////////////////////////

  bool ready = false;

  bool stable = false;

  float noise = 0.0f;

  //////////////////////////////////////////////////////////////////////////
  // CONTROLE RUÍDO
  //////////////////////////////////////////////////////////////////////////

  static const uint8_t NOISE_SAMPLES = 10;

  float noiseBuffer[NOISE_SAMPLES];

  uint8_t noiseIndex = 0;

public:
  //////////////////////////////////////////////////////////////////////////
  // CONSTRUTOR
  //////////////////////////////////////////////////////////////////////////

  SensorBalanca(HardwareSerial &porta)
  {
    serial = &porta;

    for (uint8_t i = 0; i < NOISE_SAMPLES; i++)
    {
      noiseBuffer[i] = 0.0f;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // LEITURA SENSOR
  //////////////////////////////////////////////////////////////////////////

  bool leitura()
  {
    while (serial->available())
    {
      char c = serial->read();

      //////////////////////////////////////////////////////////////////////////
      // IGNORA CR
      //////////////////////////////////////////////////////////////////////////

      if (c == '\r')
        continue;

      //////////////////////////////////////////////////////////////////////////
      // FIM LINHA
      //////////////////////////////////////////////////////////////////////////

      if (c == '\n')
      {
        rxBuffer[rxIndex] = '\0';

        valorLido = atof(rxBuffer);

        rxIndex = 0;

        //////////////////////////////////////////////////////////////////////////
        // IGNORA ZERO
        //////////////////////////////////////////////////////////////////////////

        if (fabs(valorLido) < 0.0001f)
        {
          ready = false;
          return false;
        }

        //////////////////////////////////////////////////////////////////////////
        // PESO
        //////////////////////////////////////////////////////////////////////////

        pesoGramas = balanca.get_units(valorLido);

        pesoKg = pesoGramas / 1000.0f;

        //////////////////////////////////////////////////////////////////////////
        // ESTABILIDADE
        //////////////////////////////////////////////////////////////////////////

        analisarRuido();

        ready = true;

        return true;
      }

      //////////////////////////////////////////////////////////////////////////
      // BUFFER
      //////////////////////////////////////////////////////////////////////////

      else
      {
        if (rxIndex < sizeof(rxBuffer) - 1)
        {
          rxBuffer[rxIndex++] = c;
        }
      }
    }

    return false;
  }

private:
  //////////////////////////////////////////////////////////////////////////
  // ANÁLISE DE RUÍDO
  //////////////////////////////////////////////////////////////////////////

  void analisarRuido()
  {
    noiseBuffer[noiseIndex] = valorLido;

    noiseIndex++;

    if (noiseIndex >= NOISE_SAMPLES)
    {
      noiseIndex = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // MIN/MAX
    //////////////////////////////////////////////////////////////////////////

    float minV = noiseBuffer[0];
    float maxV = noiseBuffer[0];

    for (uint8_t i = 1; i < NOISE_SAMPLES; i++)
    {
      if (noiseBuffer[i] < minV)
        minV = noiseBuffer[i];

      if (noiseBuffer[i] > maxV)
        maxV = noiseBuffer[i];
    }

    //////////////////////////////////////////////////////////////////////////
    // RUÍDO
    //////////////////////////////////////////////////////////////////////////

    noise = maxV - minV;

    //////////////////////////////////////////////////////////////////////////
    // ESTABILIDADE
    //////////////////////////////////////////////////////////////////////////

    stable = (noise < 0.020f);
  }

public:
  //////////////////////////////////////////////////////////////////////////
  // STATUS
  //////////////////////////////////////////////////////////////////////////

  bool isReady()
  {
    return ready;
  }

  bool isStable()
  {
    return stable;
  }

  float getNoise()
  {
    return noise;
  }

  //////////////////////////////////////////////////////////////////////////
  // DADOS
  //////////////////////////////////////////////////////////////////////////

  float getRaw()
  {
    return valorLido;
  }

  float getKg()
  {
    return pesoKg;
  }

  float getGramas()
  {
    return pesoGramas;
  }

  //////////////////////////////////////////////////////////////////////////
  // TARA
  //////////////////////////////////////////////////////////////////////////

  void tare()
  {
    balanca.tare(valorLido);

    Serial.println("--------------------------------");
    Serial.println("BALANCA ZERADA");
    Serial.println("--------------------------------");
  }

  //////////////////////////////////////////////////////////////////////////
  // CALIBRAÇÃO
  //////////////////////////////////////////////////////////////////////////

  void calibra(float pesoConhecido)
  {
    if (!stable)
    {
      Serial.println("ERRO: BALANCA INSTAVEL");
      return;
    }

    balanca.calibra(valorLido, pesoConhecido);

    Serial.println("--------------------------------");

    Serial.print("BALANCA CALIBRADA COM ");

    Serial.print(pesoConhecido);

    Serial.println(" g");

    Serial.println("--------------------------------");
  }
};

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

  json += ",";

  json += "\"noise\":" + String(sensor1.getNoise(), 5);

  json += ",";

  json += "\"stable\":";

  json += sensor1.isStable() ? "true" : "false";

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
  if (sensor1.leitura())
  {
    pesoAtual = sensor1.getKg();

    //////////////////////////////////////////////////////////////////////////
    // DEBUG
    //////////////////////////////////////////////////////////////////////////

    Serial.print("RAW: ");

    Serial.print(sensor1.getRaw(), 3);

    Serial.print(" | KG: ");

    Serial.print(sensor1.getKg(), 3);

    Serial.print(" | Noise: ");

    Serial.print(sensor1.getNoise(), 5);

    Serial.print(" | Stable: ");

    if (sensor1.isStable())
      Serial.print("YES");
    else
      Serial.print("NO");

    Serial.println();
  }

  updateDisplay();
  while (Serial.available())
  {
    //////////////////////////////////////////////////////////////////////////
    // COMANDOS
    //////////////////////////////////////////////////////////////////////////

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
        delay(3000);

        sensor1.calibra(pesoCalibracao);

        break;
      }
    }
  }

  if (Serial.available())
  {
    String comando = Serial.readStringUntil('\n');

    comando.trim();

    if (comando.equalsIgnoreCase("t"))
    {
      sensor1.tare();
    }

    //////////////////////////////////////////////////////////////////////////
    // CALIBRAÇÃO
    //////////////////////////////////////////////////////////////////////////

    else if (comando.startsWith("c"))
    {
      String pesoTexto = comando.substring(1);

      float pesoConhecido = pesoTexto.toFloat();

      if (pesoConhecido <= 0)
      {
        Serial.println("ERRO: peso invalido");

        return;
      }

      Serial.println("--------------------------------");

      Serial.println("AGUARDE ESTABILIZAR...");

      Serial.println("--------------------------------");

      delay(3000);

      sensor1.calibra(pesoConhecido);
    }
  }

  delay(5);
}