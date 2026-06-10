#pragma once

#include <Arduino.h>

extern float pesoAtual;
extern float ttotal;
extern int contEixo;
extern String sta_ssid;
extern String sta_password;
extern String ap_ssid;
extern String ap_password;
extern String tplaca;
extern bool g_wifiConnected;
extern bool g_apMode;
extern String tabela[20][11];

extern float offset;
extern float scale_factor;
extern float sensorKnownWeight;
extern int sensorId;
extern float pesoConhecido[4];
extern float fatorEscalaConhecido[4];
//xtern float defaultKnownWeight[4];
//extern float defaultScaleFactor[4];

void salvarComEEPROM();
void carregarComEEPROM();
