#include "HX711_Module.hpp"
#include "EEPROM_Module.hpp"

//const float HX711::defaultKnownWeight[4] = {84000.0f, 84000.0f, 84000.0f, 84000.0f};
//const float HX711::defaultScaleFactor[4] = {-6259.31f,-5201.24f, -5420.0f, -5400.0f};

int sensorIndex[4] = {0, 1, 2, 3};
float novoFator[4] = {0.0f, 0.0f, 0.0f, 0.0f};
float scale_factor = 0.0f;

static bool fatorEscalaValido(float fator)
{
    return !isnan(fator) && !isinf(fator) && fabs(fator) > 0.0001f;
}

/////////////////////////////////////////////////////////////////////////////
// CONSTRUTOR HX711
/////////////////////////////////////////////////////////////////////////////

HX711::HX711()
    : HX711(0)
{
}

HX711::HX711(int sensorSlot)
{
    if (sensorSlot < 0 || sensorSlot >= 4) {
        sensorSlot = 0;
    }

    int mappedIndex = ::sensorIndex[sensorSlot];
    if (mappedIndex < 0 || mappedIndex >= 4) {
        mappedIndex = 0;
    }

    offset = 0.0f;
    sensorKnownWeight = pesoConhecido[mappedIndex];
    sensorId = mappedIndex;
    sensorScaleFactor = fatorEscalaValido(fatorEscalaConhecido[mappedIndex]) ? fatorEscalaConhecido[mappedIndex] : 1.0f;

    //////////////////////////////////////////////////////////////////////////
    // FATOR PADRÃO
    //////////////////////////////////////////////////////////////////////////

    ::scale_factor = sensorScaleFactor;
}

/////////////////////////////////////////////////////////////////////////////
// TARA
/////////////////////////////////////////////////////////////////////////////

void HX711::tare(float leituraAtual, int sensorSlot)
{
    if (sensorSlot < 0 || sensorSlot >= 4) {
        sensorSlot = 0;
    }

    int mappedIndex = ::sensorIndex[sensorSlot];
    if (mappedIndex < 0 || mappedIndex >= 4) {
        mappedIndex = 0;
    }

    sensorId = mappedIndex;
    offset = leituraAtual;
    sensorKnownWeight = pesoConhecido[mappedIndex];
    sensorScaleFactor = fatorEscalaValido(fatorEscalaConhecido[mappedIndex]) ? fatorEscalaConhecido[mappedIndex] : 1.0f;
    ::scale_factor = sensorScaleFactor;

    Serial.print("OFFSET (Sensor ");
    Serial.print(mappedIndex + 1);
    Serial.println("):");
    Serial.println(offset);
    Serial.print("Known weight:");
    Serial.println(sensorKnownWeight);
    Serial.print("Scale factor EEPROM:");
    Serial.println(sensorScaleFactor, 8);
}

/////////////////////////////////////////////////////////////////////////////
// LEITURA EM UNIDADES
/////////////////////////////////////////////////////////////////////////////

float HX711::get_units(float leituraAtual)
{
    if (!fatorEscalaValido(sensorScaleFactor))
    {
        sensorScaleFactor = fatorEscalaValido(fatorEscalaConhecido[sensorId]) ? fatorEscalaConhecido[sensorId] : 1.0f;
    }

    return (leituraAtual - offset) * sensorScaleFactor;
}

/////////////////////////////////////////////////////////////////////////////
// CALIBRAÇÃO
/////////////////////////////////////////////////////////////////////////////

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

    sensorScaleFactor = known_weight / leituraLiquida;
    ::scale_factor = sensorScaleFactor;
    Serial.print("Scale factor calculado:");
    Serial.println(::scale_factor, 8);

    int calibIndex = sensorId;

    // Atualiza os dados para o sensor específico calibrado.
    // O índice corresponde ao sensor 1, 2, 3 ou 4 como 0, 1, 2 ou 3.
    if (calibIndex >= 0 && calibIndex < 4) {
        novoFator[calibIndex] = sensorScaleFactor;
        fatorEscalaConhecido[calibIndex] = sensorScaleFactor;
        pesoConhecido[calibIndex] = known_weight;
        sensorKnownWeight = known_weight;
    }

    //////////////////////////////////////////////////////////////////////////
    // RESULTADO
    //////////////////////////////////////////////////////////////////////////

    Serial.print("FACTOR (Sensor "); Serial.print(sensorId); Serial.print("): ");
    Serial.println(::scale_factor, 8);

    Serial.println("--------------------------------");
}

void HX711::setScale(float scale)
{
    sensorScaleFactor = fatorEscalaValido(scale) ? scale : 1.0f;
    ::scale_factor = sensorScaleFactor;
}

float HX711::getScale()
{
    return sensorScaleFactor;
}

/////////////////////////////////////////////////////////////////////////////
// CONSTRUTOR SENSOR BALANÇA
/////////////////////////////////////////////////////////////////////////////

SensorBalanca::SensorBalanca(HardwareSerial &porta, String prefixoSensor, int sensorSlot)
    : serial(&porta), prefixo(prefixoSensor), balanca(sensorSlot)
{
    if (sensorSlot < 0 || sensorSlot >= 4) {
        sensorSlot = 0;
    }

    int mappedIndex = ::sensorIndex[sensorSlot];
    if (mappedIndex < 0 || mappedIndex >= 4) {
        mappedIndex = 0;
    }

    sensorIndex = mappedIndex;
}

/////////////////////////////////////////////////////////////////////////////
// PROCESSA STRING
/////////////////////////////////////////////////////////////////////////////

bool SensorBalanca::processaString(String s)
{
    if (s.startsWith(prefixo))
    {
        float rawValue = atof(s.c_str() + prefixo.length());

        // Se o valor recebido raw for igual a zero, deve ser ignorado.
        if (rawValue == 0.0f)
        {
            return false;
        }

        valorLido = rawValue;

        // Apenas marca como pronto se o valor é válido.
        // Não descartamos o valor lido aqui para permitir a tara de valores brutos.
        if (fabs(valorLido) < 0.0001f)
        {
            ready = false;
        }
        else
        {
            ready = true;
        }

        pesoGramas = balanca.get_units(valorLido);
        pesoKg = pesoGramas / 1000.0f;
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////
// STATUS
/////////////////////////////////////////////////////////////////////////////

bool SensorBalanca::isReady()
{
    return ready;
}

bool SensorBalanca::leitura()
{
    if (serial->available())
    {
        String s = serial->readStringUntil('\n');
        return processaString(s);
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////
// RAW
/////////////////////////////////////////////////////////////////////////////

float SensorBalanca::getRaw()
{
    return valorLido;
}

/////////////////////////////////////////////////////////////////////////////
// KG
/////////////////////////////////////////////////////////////////////////////

float SensorBalanca::getKg()
{
    return pesoKg;
}

/////////////////////////////////////////////////////////////////////////////
// GRAMAS
/////////////////////////////////////////////////////////////////////////////

float SensorBalanca::getGramas()
{
    return pesoGramas;
}

/////////////////////////////////////////////////////////////////////////////
// TARA
/////////////////////////////////////////////////////////////////////////////

bool SensorBalanca::tare()
{
    // Zeragem forçada: ignora verificação de ready
    balanca.tare(valorLido, sensorIndex);

    // Forçar recalculamento imediato após definir novo offset
    pesoGramas = balanca.get_units(valorLido);
    pesoKg = pesoGramas / 1000.0f;

    Serial.println("--------------------------------");
    Serial.println("BALANCA ZERADA (FORCADA)");
    Serial.print("Novo Valor (KG): ");
    Serial.println(pesoKg, 3);
    Serial.println("--------------------------------");
    return true;
}

/////////////////////////////////////////////////////////////////////////////
// CALIBRAÇÃO
/////////////////////////////////////////////////////////////////////////////

bool SensorBalanca::calibra(float pesoConhecido)
{
    if (!ready)
    {
        Serial.print("ERRO: Sensor ");
        Serial.print(sensorIndex + 1);
        Serial.println(" nao pronto para calibracao");
        return false;
    }

    balanca.calibra(valorLido, pesoConhecido);

    Serial.println("--------------------------------");

    Serial.print("BALANCA CALIBRADA COM ");

    Serial.print(pesoConhecido);

    Serial.println(" g");

    Serial.println("--------------------------------");
    return true;
}

void SensorBalanca::setScale(float scale)
{
    balanca.setScale(scale);
}

float SensorBalanca::getScale()
{
    return balanca.getScale();
}
