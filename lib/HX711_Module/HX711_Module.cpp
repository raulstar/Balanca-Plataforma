#include "HX711_Module.hpp"

// variável global vinda do main.cpp
// extern float valorLido;

// peso atual global
float plataforma1 = 0.0;

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

float HX711::get_units(float leituraAtual)
{
    plataforma1 = (leituraAtual - offset) * scale_factor;

    return plataforma1;
}

// CALIBRAÇÃO
void HX711::calibra(float leituraAtual, float known_weight)
{
    //////////////////////////////////////////////////////////////////////////
    // LEITURA LÍQUIDA
    //////////////////////////////////////////////////////////////////////////

    float leituraLiquida = leituraAtual - offset;

    //////////////////////////////////////////////////////////////////////////
    // DEBUG
    //////////////////////////////////////////////////////////////////////////

    Serial.println("--------------------------------");

    Serial.print("OFFSET: ");
    Serial.println(offset, 6);

    Serial.print("LEITURA: ");
    Serial.println(leituraAtual, 6);

    Serial.print("LIQUIDA: ");
    Serial.println(leituraLiquida, 6);

    //////////////////////////////////////////////////////////////////////////
    // PROTEÇÃO DIVISÃO ZERO
    //////////////////////////////////////////////////////////////////////////

    if (fabs(leituraLiquida) < 0.0001f)
    {
        Serial.println("ERRO: PESO INVALIDO");

        return;
    }

    //////////////////////////////////////////////////////////////////////////
    // CALIBRAÇÃO
    //////////////////////////////////////////////////////////////////////////

    scale_factor = known_weight / leituraLiquida;

    //////////////////////////////////////////////////////////////////////////
    // RESULTADO
    //////////////////////////////////////////////////////////////////////////

    Serial.print("FACTOR: ");
    Serial.println(scale_factor, 8);

    Serial.println("--------------------------------");
}