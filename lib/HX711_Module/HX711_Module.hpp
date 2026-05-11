#ifndef HX711_MODULE_HPP
#define HX711_MODULE_HPP

#include <Arduino.h>

class HX711
{
private:
    float offset;
    float scale_factor;

public:
    HX711();

    // tara usando leitura atual
    void tare(float leituraAtual);

    // leitura convertida
    float get_units();

    // calibração
    void calibra(float known_weight);
};

#endif