#include "Thermal_Printer.hpp"

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

void imprimirRegistro()
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

    printer->boldOff();

    printer->setSize('S');

    printer->println("--------------------------------");

    printer->justify('L');

    printer->print("Registro: ");
    printer->println(contadorRegistro);

    printer->print("Data: ");
    printer->println(tdata);

    printer->print("Hora: ");
    printer->println(thora);

    printer->print("Placa: ");
    printer->println(placaVeiculo);

    printer->println("--------------------------------");
     printer->boldOn();
    printer->print("Peso Total: ");
    printer->print(ttotal, 2);
    printer->println(" kg");
     printer->boldOff();

    printer->print("Tara: ");
    printer->print(ttara, 2);
    printer->println(" kg");

    printer->print("Qtd Eixos: ");
    printer->println(eixo);

    printer->println("--------------------------------");

    printer->println("PESO POR EIXO");

    printer->print("Eixo 1: ");
    printer->print(peixo1);
    printer->println(" kg");
    if (peixo2 > 0)
    {
        printer->print("Eixo 2: ");
        printer->print(peixo2);
        printer->println(" kg");
    }
    if (peixo3 > 0)
    {
        printer->print("Eixo 3: ");
        printer->print(peixo3);
        printer->println(" kg");
    }
    if (peixo4 > 0)
    {
        printer->print("Eixo 4: ");
        printer->print(peixo4);
        printer->println(" kg");
    }
    if (peixo5 > 0)
    {
        printer->print("Eixo 5: ");
        printer->print(peixo5);
        printer->println(" kg");
    }
    if (peixo6 > 0)
    {
        printer->print("Eixo 6: ");
        printer->print(peixo6);
        printer->println(" kg");
    }
    printer->println("--------------------------------");

    printer->justify('C');

    printer->boldOn();
    printer->println("REVLO");
    printer->println("Sistema de Pesagem");
    printer->boldOff();
     printer->println("revlo.com.br/");

    printer->feed(4);
}