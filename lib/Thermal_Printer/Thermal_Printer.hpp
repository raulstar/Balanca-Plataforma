#ifndef THERMAL_PRINTER_HPP
#define THERMAL_PRINTER_HPP

#include <Arduino.h>
#include "Adafruit_Thermal.h"

// ===========================
// VARIÁVEIS GLOBAIS
// ===========================

extern String placaVeiculo;

extern String dataRegistro;

extern String tdata;
extern String thora;

extern int contadorRegistro;

extern float ttotal;
extern float ttara;

extern int eixo;

extern int peixo1;
extern int peixo2;
extern int peixo3;
extern int peixo4;
extern int peixo5;
extern int peixo6;

// ===========================
// FUNÇÕES
// ===========================

void iniciarImpressora(
    HardwareSerial &serial,
    int baudrate,
    int rxPin,
    int txPin
);

void imprimirRegistro();

#endif