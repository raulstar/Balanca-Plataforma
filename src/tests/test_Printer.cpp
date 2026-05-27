#include <Arduino.h>
#include "Thermal_Printer.hpp"


// ===========================
// DEFINIÇÃO DAS VARIÁVEIS
// ===========================

String placaVeiculo;

String dataRegistro;

String tdata;
String thora;

int contadorRegistro;

float ttotal;
float ttara;

int eixo;

int peixo1;
int peixo2;
int peixo3;
int peixo4;
int peixo5;
int peixo6;

// ===========================
// SERIAL DA IMPRESSORA
// ===========================

HardwareSerial impressoraSerial(0);

// ===========================

void setup() {

    Serial.begin(115200);

    // ===========================
    // INICIA IMPRESSORA
    // ===========================

    iniciarImpressora(
        impressoraSerial,
        9600,
        4,
        5
    );

    // ===========================
    // DADOS DE TESTE
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

    // ===========================
    // TESTE DE IMPRESSÃO
    // ===========================

    DadosImpressao dados;
    dados.placa = placaVeiculo;
    dados.data = dataRegistro;
    dados.hora = thora;
    dados.contador = contadorRegistro;
    dados.total = ttotal;
    dados.tara = ttara;
    dados.eixo = eixo;
    dados.peixo1 = peixo1;
    dados.peixo2 = peixo2;
    dados.peixo3 = peixo3;
    dados.peixo4 = peixo4;
    dados.peixo5 = peixo5;
    dados.peixo6 = peixo6;

    imprimirRegistro(dados);
}

void loop() {

}