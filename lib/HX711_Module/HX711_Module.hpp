#ifndef HX711_Module
#define HX711_Module

#include <stdint.h> 

class HX711 {
private:
    float offset;       // valor de tara
    float scale_factor; // fator de calibração

public:
    HX711();

    void tare();                  // zera a balança
    float get_units(uint8_t times = 1); // retorna valor calibrado
    void calibra(float known_weight);   // calibração com peso conhecido
};

// variável global com valor já processado
extern float pesoAtual;

#endif