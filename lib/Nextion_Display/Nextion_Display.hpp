#pragma once

#include <Arduino.h>
#include "../../include/config.hpp"
#include "../../lib/HX711_Module/HX711_Module.hpp"
#include "../../lib/WiFi_Server/WiFi_Server.hpp"

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
extern bool salvarRegistro;
extern volatile bool imprimir;
extern SemaphoreHandle_t xSensorMutex;
extern String calib;

// ======================================================
// WiFi Configuration Variables
// ======================================================
extern String sta_ssid;
extern String sta_password;
extern String ap_ssid;
extern String ap_password;
extern bool g_wifiConnected;
extern bool g_apMode;

// ======================================================
// DATA / HORA
// ======================================================
extern String tdata;
extern String thora;
extern String tbateria;
extern String bplatafor1;
extern String bplatafor2;
extern String bplatafor3;
extern String bplatafor4;
extern int indexCalib;
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
extern int eixo1;
extern int eixo2;
extern int eixo3;
extern int eixo4;
extern int eixo5;
extern int peixo1;
extern int peixo2;
extern int peixo3;
extern int peixo4;
extern int peixo5;
extern int peixo6;

// ======================================================
// MATRIZ DE REGISTROS
// ======================================================
extern String tabela[20][11];

// ======================================================
// CONTADORES
// ======================================================
extern int linhaAtual;
extern int contadorRegistro;
extern int contEixo;
extern int eixo;
extern int peixo;

// ======================================================
// FUNÇÕES
// ======================================================
void initNextion();

void setSensores(SensorConfig* sens, int num);

void updateDisplay();

void processNextionCommands();

void setNextionText(String objName, String value);

void handle_bsom();

void handle_bzero();

void handle_bsalvar();

void handle_blimpar();

void handle_bgeneric(String cmd);

void handle_bcalib(String cmd);
