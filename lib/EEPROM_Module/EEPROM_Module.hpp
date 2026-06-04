#pragma once

#include <Arduino.h>

extern float pesoAtual;
extern float ttotal;
extern String sta_ssid;
extern String sta_password;
extern String ap_ssid;
extern String ap_password;
extern bool g_wifiConnected;
extern bool g_apMode;
extern String tabela[20][6];

extern float offset;
extern float scale_factor;
extern float sensorKnownWeight;
extern int sensorId;
extern float defaultKnownWeight[4];
extern float defaultScaleFactor[4];

void salvarComEEPROM();
void carregarComEEPROM();
