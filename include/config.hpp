#pragma once
#include <Arduino.h>
#include "../lib/HX711_Module/HX711_Module.hpp"

struct SensorConfig
{
  SensorBalanca *sensor;
  String prefixo;
};
