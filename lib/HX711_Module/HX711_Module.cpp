#include "HX711_Module.hpp"


// variável global vinda do main.cpp
extern float valorLido;

// variável global de saída
float pesoAtual = 0.0;

// construtor
HX711::HX711() {
    offset = 0.0;
    scale_factor = 1.0;
}

// função de tara
void HX711::tare() {
    offset = valorLido;
}

// média das leituras (mesmo que seja o mesmo valor, mantém compatibilidade)
float HX711::get_units(uint8_t times) {
    float soma = 0;

    for (uint8_t i = 0; i < times; i++) {
        soma += valorLido;
    }

    float media = soma / times;

    pesoAtual = (media - offset) / scale_factor;

    return pesoAtual;
}

// calibração usando peso conhecido
void HX711::calibra(float known_weight) {
    float leitura = valorLido - offset;

    if (known_weight != 0) {
        scale_factor = leitura / known_weight;
    }
}