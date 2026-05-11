#include "HX711_Module.hpp"

// variável global vinda do main.cpp
extern float valorLido;

// peso atual global
float pesoAtual = 0.0;

// construtor
HX711::HX711()
{
    offset = 0.0;

    // começa sem calibração
    scale_factor = 1;
}

// TARA
void HX711::tare(float leituraAtual)
{
    offset = leituraAtual;

    Serial.println("OFFSET:");
    Serial.println(offset);
}

float HX711::get_units()
{
    pesoAtual = (valorLido - offset) * scale_factor;

    return pesoAtual;
}

// CALIBRAÇÃO
void HX711::calibra(float known_weight)
{
    float leituraLiquida = valorLido - offset;

    if (known_weight != 0)
    {
        scale_factor = known_weight / leituraLiquida;
    }

    Serial.println("FACTOR:");
    Serial.println(scale_factor, 8);
}