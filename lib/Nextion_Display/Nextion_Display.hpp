#pragma once
#include <Arduino.h>

// UART
extern HardwareSerial NEXTION_SERIAL;

// funções
void initNextion();
void nextionCmd(const String &cmd);
void lerNextion();

// 🔴 callback externo (definido no main)
void handleZero();