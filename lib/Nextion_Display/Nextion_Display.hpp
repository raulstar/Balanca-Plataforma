#pragma once

#include <Arduino.h>

// ======================================================
// CONFIGURAÇÃO UART NEXTION
// ======================================================
#define NEXTION_RX 25
#define NEXTION_TX 26

extern HardwareSerial NEXTION_SERIAL;

// ======================================================
// VARIÁVEIS GLOBAIS
// ======================================================
extern String placaVeiculo;
extern String dataRegistro;
extern float pesoCalibracao1;
extern bool calibrando1;
extern bool zero;
extern bool imprimir;

// ======================================================
// DATA / HORA
// ======================================================
extern String tdata;
extern String thora;
extern String tbateria;

// ======================================================
// PLACA
// ======================================================
extern String tplaca;
extern String placa;

// ======================================================
// VARIÁVEIS NUMÉRICAS
// ======================================================
extern float pesoAtual;
extern float ttotal;
extern float ttara;

// ======================================================
// MATRIZ DE REGISTROS
// ======================================================
extern String tabela[20][6];

// ======================================================
// CONTADORES
// ======================================================
extern int linhaAtual;
extern int contadorRegistro;
extern int eixo;
extern int peixo;

// ======================================================
// FUNÇÕES
// ======================================================
void initNextion();

void updateDisplay();

void processNextionCommands();

void setNextionText(String objName, String value);

void handle_bsom();

void handle_bzero();

void handle_bsalvar();

void handle_blimpar();

void handle_bgeneric(String cmd);

void processNextionCommands();

void handle_bcalib(String cmd);