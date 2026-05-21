#include "Adafruit_Thermal.h"

// SERIAL DA IMPRESSORA
HardwareSerial impressoraSerial(2);

// RX, TX
Adafruit_Thermal printer(&impressoraSerial);

// ===========================
// VARIÁVEIS GLOBAIS
// ===========================

String placaVeiculo;
String dataRegistro;
String tdata;
String thora;

int contadorRegistro;

float ttotal;   // peso total
float ttara;    // tara

int eixo;       // número de eixos

int peixo1;
int peixo2;
int peixo3;
int peixo4;
int peixo5;
int peixo6;

// ===========================

void setup() {

  Serial.begin(115200);

  // SERIAL DA IMPRESSORA
  impressoraSerial.begin(19200, SERIAL_8N1, 16, 17);

  printer.begin();

  // ===========================
  // DADOS DE EXEMPLO
  // ===========================

  placaVeiculo = "BRA2E19";

  dataRegistro = "21/05/2026";

  tdata = "21/05/2026";
  thora = "14:32:10";

  contadorRegistro = 125;

  ttotal = 52340.50;
  ttara  = 18320.00;

  eixo = 6;

  peixo1 = 5200;
  peixo2 = 8400;
  peixo3 = 9100;
  peixo4 = 9800;
  peixo5 = 10100;
  peixo6 = 9740;

  // IMPRIME O REGISTRO
  imprimirRegistro();
}

void loop() {

}

// ===========================
// FUNÇÃO DE IMPRESSÃO
// ===========================

void imprimirRegistro() {

  printer.justify('C');

  printer.boldOn();
  printer.setSize('M');

  printer.println("REGISTRO DE");
  printer.println("PESAGEM");

  printer.boldOff();

  printer.setSize('S');

  printer.println("--------------------------------");

  printer.justify('L');

  printer.print("Registro: ");
  printer.println(contadorRegistro);

  printer.print("Data: ");
  printer.println(tdata);

  printer.print("Hora: ");
  printer.println(thora);

  printer.print("Placa: ");
  printer.println(placaVeiculo);

  printer.println("--------------------------------");

  printer.print("Peso Total: ");
  printer.print(ttotal, 2);
  printer.println(" kg");

  printer.print("Tara: ");
  printer.print(ttara, 2);
  printer.println(" kg");

  printer.print("Qtd Eixos: ");
  printer.println(eixo);

  printer.println("--------------------------------");

  printer.println("PESO POR EIXO");

  printer.print("Eixo 1: ");
  printer.print(peixo1);
  printer.println(" kg");

  printer.print("Eixo 2: ");
  printer.print(peixo2);
  printer.println(" kg");

  printer.print("Eixo 3: ");
  printer.print(peixo3);
  printer.println(" kg");

  printer.print("Eixo 4: ");
  printer.print(peixo4);
  printer.println(" kg");

  printer.print("Eixo 5: ");
  printer.print(peixo5);
  printer.println(" kg");

  printer.print("Eixo 6: ");
  printer.print(peixo6);
  printer.println(" kg");

  printer.println("--------------------------------");

  printer.justify('C');

  printer.println("Sistema de Pesagem");
  printer.println("ESP32 + Impressora Termica");

  printer.feed(4);
}