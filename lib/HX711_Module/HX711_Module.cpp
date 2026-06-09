#include "HX711_Module.hpp"

const float HX711::defaultKnownWeight[4] = {84000.0f, 84000.0f, 84000.0f, 84000.0f};
const float HX711::defaultScaleFactor[4] = {-5412.36425781f, -5410.0f, -5420.0f, -5400.0f};

int sensorIndex[4] = {0, 1, 2, 3};
float novoFator[4] = {0.0f, 0.0f, 0.0f, 0.0f};
float scale_factor = 0.0f;

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
    sensorKnownWeight = defaultKnownWeight[mappedIndex];
    sensorId = mappedIndex;

    //////////////////////////////////////////////////////////////////////////
    // FATOR PADRÃO
    //////////////////////////////////////////////////////////////////////////

    ::scale_factor = defaultScaleFactor[mappedIndex];
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

    Serial.print("OFFSET (Sensor ");
    Serial.print(mappedIndex + 1);
    Serial.println("):");
    Serial.println(offset);
    Serial.print("Known weight:");
    Serial.println(sensorKnownWeight);
    Serial.print("Scale factor:");
    Serial.println(::scale_factor, 8);
}

/////////////////////////////////////////////////////////////////////////////
// LEITURA EM UNIDADES
/////////////////////////////////////////////////////////////////////////////

float HX711::get_units(float leituraAtual)
{
    return (leituraAtual - offset) * scale_factor;
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

    ::scale_factor = known_weight / leituraLiquida;
    
    // Atualiza o fator para o sensor específico no índice mapeado
    if (sensorId >= 0 && sensorId < 4) {
        novoFator[sensorId] = ::scale_factor;
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
    ::scale_factor = scale;
}

float HX711::getScale()
{
    return ::scale_factor;
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
        valorLido = atof(s.c_str() + prefixo.length());

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
