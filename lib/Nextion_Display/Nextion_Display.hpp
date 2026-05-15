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

// ======================================================
// DATA / HORA
// ======================================================
extern String gdata;
extern String ghora;

// ======================================================
// PLACA
// ======================================================
extern String gplaca;

// ======================================================
// VARIÁVEIS NUMÉRICAS
// ======================================================
extern float pesoAtual;
extern float ttotal;
extern float gtara;

// ======================================================
// MATRIZ DE REGISTROS
// ======================================================
extern String tabela[20][6];

// ======================================================
// CONTADORES
// ======================================================
extern int linhaAtual;
extern int contadorRegistro;

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