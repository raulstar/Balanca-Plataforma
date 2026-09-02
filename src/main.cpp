#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <cmath> // for fabs

#include "WiFi_Server.hpp"
#include "Nextion_Display.hpp"
#include "HX711_Module.hpp"
#include "Thermal_Printer.hpp"
#include <SoftwareSerial.h>
#include "config.hpp"
#include "EEPROM_Module.hpp"
//#include "BasicOTA.hpp"

#define M0 19
#define M1 21
#define OK 32

#define ok digitalRead(OK)
#define m1 digitalWrite(M1, LOW)
#define m0 digitalWrite(M0, LOW)
/////////////////////////////////////////////////////////////////////////////
// VARIÁVEIS GLOBAIS

uint32_t displayUpdateInterval = 500;
HardwareSerial SerialPort(2);
SoftwareSerial impressoraSerial(4, 5);

//BasicOTA OTA;
float pesoCalibracao1 = 84000.0f;
// "div" conflita com a função div() de stdlib.h; o macro mantém o uso compatível no firmware.
#define div divPeso
int div = 5;
extern volatile bool imprimir;
bool useAP = false; // altere para false para usar modo estação

// Mutex to protect sensor array access
// Defined in Nextion_Display.cpp so it is available to both firmware and unit tests.
// SemaphoreHandle_t xSensorMutex = nullptr;
// Task handle for tare task
TaskHandle_t hTareTask = nullptr;
// Task handle for serial processing task
TaskHandle_t hSerialTask = nullptr;
// Task handle for Nextion display commands task
TaskHandle_t hNextionTask = nullptr;
// Task handle for SerialPort reading task
TaskHandle_t hPortTask = nullptr;

void taskUpdateDisplay(void *pvParameters)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(displayUpdateInterval);
  ESP_LOGI("Task", "Display update task started with interval %d ms", displayUpdateInterval);
  for (;;)
  {
    tplaca = placaVeiculo;
    //thora = "14:30";
    //tdata = "15/05/2026";
    ttara = ttotal;
    updateDisplay();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

SensorBalanca sensor1(SerialPort, "S1", 0);
SensorBalanca sensor2(SerialPort, "S2", 1);
SensorBalanca sensor3(SerialPort, "S3", 2);
SensorBalanca sensor4(SerialPort, "S4", 3);

SensorConfig sensores[] = {
    {&sensor1, "S1"},
    {&sensor2, "S2"},
    {&sensor3, "S3"},
    {&sensor4, "S4"}};
int numSensores = sizeof(sensores) / sizeof(sensores[0]);

// Legacy wrapper – now just notifies the async tare task.
void tareAllSensors()
{
  if (hTareTask)
  {
    // Notify the tare task to run once.
    xTaskNotifyGive(hTareTask);
  }
}

void taskTareAllSensors(void *pvParameters)
{
  for (;;)
  {
    // Wait for a notification from tareAllSensors()
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Protect the sensor array while performing tare
    if (xSensorMutex && xSemaphoreTake(xSensorMutex, portMAX_DELAY) == pdTRUE)
    {
      for (int i = 0; i < numSensores; ++i)
      {
        // O valorLido é mantido na instância de SensorBalanca.
        // A tara so acontece se o sensor estiver estavel; caso contrario a
        // propria tare() informa o motivo.
        sensores[i].sensor->tare();
      }
      xSemaphoreGive(xSensorMutex);
      ESP_LOGI("TareTask", "Todas as balanças zeradas (async).");
    }
  }
}

void handleCalibrar() // CALIBRAÇÃO WEB
{
  server.send(200, "text/plain", "Modo calibração iniciado");
}

void handleSomar() // SOMAR WEB
{
  handle_bsom();
  server.send(200, "text/plain", "Peso somado");
}

void taskProcessarImpressao(void *pvParameters)
{
  for (;;)
  {
    if (imprimir)
    {
      DadosImpressao dados;
      dados.placa = placaVeiculo;
      dados.data = tdata;
      dados.hora = thora;
      dados.contador = contadorRegistro;
      dados.total = ttotal;
      dados.tara = ttara;
      eixo = contEixo;
      dados.eixo = eixo;
      dados.peixo1 = peixo1;
      dados.peixo2 = peixo2;
      dados.peixo3 = peixo3;
      dados.peixo4 = peixo4;
      dados.peixo5 = peixo5;
      dados.peixo6 = peixo6;

      imprimirRegistro(dados);

      imprimir = false;
      Serial.println("Registro impresso.");
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void taskProcessNextionCommands(void *pvParameters)
{
  for (;;)
  {
    // Process any pending commands from Nextion display
    processNextionCommands();
    // Yield to other tasks
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void taskHandleWeb(void *pvParameters)
{
  for (;;)
  {
    // Process any pending HTTP client requests
    handleWeb();
    // Small delay to yield CPU time to other tasks
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void taskSerialPortReader(void *pvParameters)
{
  static String bufferSerial;
  for (;;)
  {
    // Read all available characters
    while (SerialPort.available())
    {
      char c = SerialPort.read();
      if (c == '\n')
      {
        // Process completed line under mutex protection
        if (xSensorMutex && xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
          for (int i = 0; i < numSensores; i++)
          {
            if (sensores[i].sensor->processaString(bufferSerial))
            {
              Serial.print(sensores[i].prefixo + " RAW: ");
              Serial.print(sensores[i].sensor->getRaw(), 3);
              Serial.print(" | FILT: ");
              Serial.print(sensores[i].sensor->getRawFiltrado(), 3);
              Serial.print(" | " + sensores[i].prefixo + " KG: ");
              Serial.print(sensores[i].sensor->getKg(), 3);
              // O valor bruto carrega o offset mecanico da plataforma: sem
              // tara o peso exibido nao tem significado.
              if (i < 4 && !sensorTarado[i])
              {
                Serial.print(" [SEM TARA]");
              }
              if (!sensores[i].sensor->estavel())
              {
                Serial.print(" [INSTAVEL]");
              }
              Serial.println();
            }
          }

          float currentPeso = 0.0f;
          for (int i = 0; i < numSensores; i++)
          {
            // Um sensor mudo congela o ultimo peso e continuaria somando um
            // valor velho; um sensor nao tarado ainda carrega o offset
            // mecanico da plataforma, que sao toneladas de peso inexistente.
            if (!sensores[i].sensor->isReady() ||
                !sensores[i].sensor->conectado())
            {
              continue;
            }

            if (i < 4 && !sensorTarado[i])
            {
              continue;
            }

            // Soma COM sinal. O fabs() daqui vinha da epoca do fator de
            // escala negativo e mascarava o defeito em vez de corrigi-lo:
            // um sensor com fator ou ligacao invertida exibia peso positivo
            // e entrava no total como se estivesse certo. Com a calibracao
            // correta o sinal ja sai positivo sozinho; saindo negativo, e
            // sinal de calibracao errada e tem de aparecer no visor.
            float sensorKg = sensores[i].sensor->getKg();
            if (!isnan(sensorKg) && !isinf(sensorKg))
            {
              currentPeso += sensorKg;
            }
          }
          if (div > 1)
          {
            pesoAtual = round(currentPeso / div) * div;
          }
          else
          {
            pesoAtual = roundf(currentPeso * 100.0f) / 100.0f;
          }
          Serial.print("Peso Atual: ");
          Serial.println(pesoAtual, 2);
          Serial.println();
          xSemaphoreGive(xSensorMutex);
        }
        bufferSerial = "";
      }
      else if (c != '\r')
      {
        bufferSerial += c;
      }
    }
    // No data available, yield to other tasks
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void taskProcessarSerial(void *pvParameters)
{
  static String bufferCmd = "";

  for (;;)
  {
    // Check for incoming serial data from debug console
    if (Serial.available())
    {
      String comando = Serial.readStringUntil('\n');
      comando.trim();

      if (comando.length() == 0)
      {
        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
      }

      //////////////////////////////////////////////////////////////////////////
      // TARA (t1-t4 or t for all)
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
          if (xSensorMutex && xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE)
          {
            bool zerado = sensores[sensorIndex].sensor->tare();
            xSemaphoreGive(xSensorMutex);
            if (zerado)
            {
              Serial.print("Sensor ");
              Serial.print(sensorIndex + 1);
              Serial.println(" zerado.");
            }
          }
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
        int espacoIndex = comando.indexOf(' ');
        if (espacoIndex != -1)
        {
          int sensorIndex = comando.substring(1, espacoIndex).toInt() - 1;
          float novoFator = comando.substring(espacoIndex + 1).toFloat();

          if (sensorIndex >= 0 && sensorIndex < numSensores)
          {
            if (xSensorMutex && xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
              sensores[sensorIndex].sensor->setScale(novoFator);
              // Grava: sem isto o fator informado a mao se perde no reboot,
              // quando o setup() recarrega o valor antigo da EEPROM.
              salvarComEEPROM();
              xSemaphoreGive(xSensorMutex);
              Serial.print("Sensor ");
              Serial.print(sensorIndex + 1);
              Serial.print(" fator definido e salvo: ");
              Serial.println(novoFator, 8);
            }
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
          if (xSensorMutex && xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE)
          {
            Serial.print("Sensor ");
            Serial.print(sensorIndex + 1);
            Serial.print(" fator: ");
            Serial.println(sensores[sensorIndex].sensor->getScale(), 8);
            xSemaphoreGive(xSensorMutex);
          }
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

          vTaskDelay(pdMS_TO_TICKS(3000));

          if (xSensorMutex && xSemaphoreTake(xSensorMutex, portMAX_DELAY) == pdTRUE)
          {
            sensores[sensorIndex].sensor->calibra(pesoCalibracao1);
            xSemaphoreGive(xSensorMutex);
          }
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

            vTaskDelay(pdMS_TO_TICKS(3000));

            if (xSensorMutex && xSemaphoreTake(xSensorMutex, portMAX_DELAY) == pdTRUE)
            {
              sensores[sensorIndex].sensor->calibra(pesoConhecido);
              xSemaphoreGive(xSensorMutex);
            }
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
    else
    {
      // No data available, yield to other tasks
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// SETUP
void setup()
{
  delay(250);
  pinMode(OK, INPUT);
  pinMode(M1, OUTPUT);
  pinMode(M0, OUTPUT);
  m1;
  m0;
  Serial.begin(115200);
  iniciarImpressora(impressoraSerial, 9600);

  SerialPort.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("\n=== SISTEMA BALANCA ===");

  Serial.println("Comandos:");
  Serial.println("t -> Tara");
  Serial.println("c -> Calibrar");

  Serial.println("========================");
  setAPMode(useAP);
  initWiFi();
  initWebServer();
  initNextion();
  setSensores(sensores, numSensores);
  if (useAP)
  {
    IPAddress webServerIP = WiFi.softAPIP();
    Serial.print("AP ativo – SSID: ");
    Serial.println("Balanca_AP");
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("WebServer em modo Access Point");
    Serial.print("SSID: ");
    Serial.println(ap_ssid);
    Serial.print("Senha: ");
    Serial.println(ap_password);
    Serial.print("IP: ");
    Serial.println(webServerIP);
    Serial.print("Endereco WebServer: http://");
    Serial.println(webServerIP);
  }
  else
  {
    IPAddress webServerIP = WiFi.localIP();
    Serial.println("WebServer em modo Station");
    Serial.print("SSID: ");
    Serial.println(sta_ssid);
    Serial.print("Senha: ");
    Serial.println(sta_password);
    Serial.print("IP: ");
    Serial.println(webServerIP);
    Serial.print("Endereco WebServer: http://");
    Serial.println(webServerIP);
  }

  // Create mutex for sensor array protection
  xSensorMutex = xSemaphoreCreateMutex();
  if (xSensorMutex == nullptr)
  {
    ESP_LOGE("Setup", "Failed to create sensor mutex");
  }
  // Create asynchronous tare task (priority higher than print task)
  xTaskCreate(taskTareAllSensors, "TareAllSensors", 4096, NULL, 2, &hTareTask);
  // Create asynchronous serial processing task
  xTaskCreate(taskProcessarSerial, "ProcessarSerial", 4096, NULL, 2, &hSerialTask);
  // Create asynchronous Nextion display commands task
  xTaskCreate(taskProcessNextionCommands, "ProcessNextion", 4096, NULL, 1, &hNextionTask);
  xTaskCreate(taskUpdateDisplay, "UpdateDisplay", 4096, NULL, 1, NULL);
  // A impressao bit-bang nao tolera preempcao no meio de um byte: a task fica
  // no core 1 (o WiFi vive no core 0) e com prioridade acima das tasks de web,
  // display e sensores, que antes a interrompiam durante a transmissao.
  xTaskCreatePinnedToCore(taskProcessarImpressao, "ProcessarImpressao", 4096, NULL, 3, NULL, 1);
  // Create asynchronous web handling task (priority 1)
  xTaskCreate(taskHandleWeb, "HandleWeb", 4096, NULL, 1, NULL);
  // Nao ha o que tarar aqui: nenhum pacote chegou ainda. A tara de partida
  // acontece na primeira leitura valida de cada sensor.

  server.on("/zero", handleZero);
  server.on("/dados", handleDados);
  server.on("/calibrar", handleCalibrar);
  server.on("/somar", handleSomar);
  Serial.println("\n=== SISTEMA BALANCA ===");

  Serial.println("Comandos:");
  Serial.println("t -> Tara");
  Serial.println("========================");

  // Create asynchronous SerialPort reading task
  xTaskCreate(taskSerialPortReader, "SerialPortReader", 4096, NULL, 2, &hPortTask);
  Serial.println("Loading EEPROM data...");
  carregarComEEPROM();
  // Os sensores foram construidos/tarados antes do carregamento da EEPROM e
  // por isso ainda guardam o fator de escala padrao. Aplica agora o fator
  // efetivamente salvo em cada sensor.
  sensor1.setScale(fatorEscalaConhecido[0]);
  sensor2.setScale(fatorEscalaConhecido[1]);
  sensor3.setScale(fatorEscalaConhecido[2]);
  sensor4.setScale(fatorEscalaConhecido[3]);
  // A tara nao e persistida: apos cada reinicio e obrigatorio zerar com a
  // plataforma vazia antes de pesar ou calibrar.
  Serial.println("ATENCAO: mantenha a plataforma vazia. A tara de partida sera feita na primeira leitura de cada sensor.");
  // Reenvia os registros restaurados da EEPROM para a tela Nextion
  restaurarTabelaNoDisplay();
  //OTA.begin(); // Setup settings
  // Ao iniciar o programa, o peso atual deve começar zerado,
  // mesmo que exista um valor anterior salvo na EEPROM.
  pesoAtual = 0.0f;
  Serial.print("pesoAtual = ");
  Serial.println(pesoAtual, 2);
  Serial.print("ttotal = ");
  Serial.println(ttotal, 2);
  Serial.print("sta_ssid = ");
  Serial.println(sta_ssid);
  Serial.print("sta_password = ");
  Serial.println(sta_password);
  Serial.print("ap_ssid = ");
  Serial.println(ap_ssid);
  Serial.print("ap_password = ");
  Serial.println(ap_password);
  Serial.print("g_wifiConnected = ");
  Serial.println(g_wifiConnected ? "true" : "false");
  Serial.print("g_apMode = ");
  Serial.println(g_apMode ? "true" : "false");
  Serial.print("dnsAddress = ");
  Serial.println(dnsAddress);
  Serial.print("AP ativo – SSID: ");
  Serial.println(ap_ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Endereco WebServer: http://");
  Serial.println(WiFi.softAPIP());
}

/////////////////////////////////////////////////////////////////////////////
// LOOP
void loop()
{
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
  }
  if (salvarRegistro)
  {
    salvarComEEPROM();
    salvarRegistro = false;
  }

  // As solicitacoes agendadas sao executadas a cada leitura recebida, mas o
  // prazo tambem precisa correr quando o transmissor fica mudo.
  if (xSensorMutex && xSemaphoreTake(xSensorMutex, 0) == pdTRUE)
  {
    for (int i = 0; i < numSensores; ++i)
    {
      if (sensores[i].sensor->pendente())
      {
        sensores[i].sensor->processarPendencia();
      }
    }
    xSemaphoreGive(xSensorMutex);
  }

   //OTA.handle(); 
  //////////////////////////////////////////////////////////////////////////
  // LEITURA SENSOR
  //////////////////////////////////////////////////////////////////////////

  // SerialPort reading is now handled by taskSerialPortReader.

   delay(2);
}
