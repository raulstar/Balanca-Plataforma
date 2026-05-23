#include "Thermal_Printer.hpp"
#include <Arduino.h>

// ===========================
// OBJETO DA IMPRESSORA
// ===========================

static Adafruit_Thermal *printer = nullptr;


// ===========================
// INICIALIZAÇÃO
// ===========================

void iniciarImpressora(
    HardwareSerial &serial,
    int baudrate,
    int rxPin,
    int txPin)
{

    serial.begin(baudrate, SERIAL_8N1, rxPin, txPin);

    printer = new Adafruit_Thermal(&serial);

    printer->begin();
}

// ===========================
// FUNÇÃO DE IMPRESSÃO
// ===========================

void imprimirRegistro(DadosImpressao dados)
{

    if (printer == nullptr)
    {
        return;
    }

    printer->justify('C');

    printer->boldOn();
    printer->setSize('M');

    printer->println("REGISTRO DE");
    printer->println("PESAGEM");
    vTaskDelay(10 / portTICK_PERIOD_MS);

    printer->boldOff();

    printer->setSize('S');

    printer->println("--------------------------------");
    vTaskDelay(10 / portTICK_PERIOD_MS);

    printer->justify('L');

    printer->print("Registro: ");
    printer->println(dados.contador);

    printer->print("Data: ");
    printer->println(dados.data);

    printer->print("Hora: ");
    printer->println(dados.hora);

    printer->print("Placa: ");
    printer->println(dados.placa);

    printer->println("--------------------------------");
    vTaskDelay(10 / portTICK_PERIOD_MS);
     printer->boldOn();
    printer->print("Peso Total: ");
    printer->print(dados.total, 2);
    printer->println(" kg");
     printer->boldOff();

    printer->print("Tara: ");
    printer->print(dados.tara, 2);
    printer->println(" kg");

    printer->print("Qtd Eixos: ");
    printer->println(dados.eixo);

    printer->println("--------------------------------");

    printer->println("PESO POR EIXO");

    printer->print("Eixo 1: ");
    printer->print(dados.peixo1);
    printer->println(" kg");
    if (dados.peixo2 > 0)
    {
        printer->print("Eixo 2: ");
        printer->print(dados.peixo2);
        printer->println(" kg");
    }
    if (dados.peixo3 > 0)
    {
        printer->print("Eixo 3: ");
        printer->print(dados.peixo3);
        printer->println(" kg");
    }
    if (dados.peixo4 > 0)
    {
        printer->print("Eixo 4: ");
        printer->print(dados.peixo4);
        printer->println(" kg");
    }
    if (dados.peixo5 > 0)
    {
        printer->print("Eixo 5: ");
        printer->print(dados.peixo5);
        printer->println(" kg");
    }
    if (dados.peixo6 > 0)
    {
        printer->print("Eixo 6: ");
        printer->print(dados.peixo6);
        printer->println(" kg");
    }
    printer->println("--------------------------------");
    vTaskDelay(10 / portTICK_PERIOD_MS);

    printer->justify('C');

    printer->boldOn();
    printer->println("REVLO");
    printer->println("Sistema de Pesagem");
    printer->boldOff();
     printer->println("revlo.com.br/");

    printer->feed(4);
}