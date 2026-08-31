#include "HX711_Module.hpp"
#include "EEPROM_Module.hpp"

//const float HX711::defaultKnownWeight[4] = {84000.0f, 84000.0f, 84000.0f, 84000.0f};
//const float HX711::defaultScaleFactor[4] = {-6259.31f,-5201.24f, -5420.0f, -5400.0f};

int sensorIndex[4] = {0, 1, 2, 3};
float novoFator[4] = {0.0f, 0.0f, 0.0f, 0.0f};
float scale_factor = 0.0f;

// Indica se cada sensor ja recebeu uma tara valida desde o boot.
// O transmissor envia counts brutos, com o offset mecanico da plataforma
// embutido, entao sem tara o peso indicado nao tem significado.
// Vive apenas em RAM: e sempre falso apos reiniciar.
bool sensorTarado[4] = {false, false, false, false};

// A tara de partida nao pode ser feita no setup(): nenhum pacote chegou
// ainda. Fica pendente ate a primeira leitura valida de cada sensor.
static bool autoTaraPendente[4] = {true, true, true, true};

// AJUSTE conforme a capacidade nominal da plataforma. Leituras acima disso
// nao sao peso: sao pacote corrompido na transmissao.
static const float CAPACIDADE_MAX_KG = 5000.0f;

// Variacao maxima entre dois pacotes consecutivos do mesmo sensor.
static const float SALTO_MAX_KG = 500.0f;

// Apos esta quantidade de rejeicoes seguidas por salto, o proximo valor e
// aceito. Evita que o sensor trave caso a referencia fique defasada.
static const uint8_t REJEICOES_ATE_RESSINCRONIZAR = 3;

// Buffer de 3 amostras por sensor para a mediana.
static float amostras[4][3];
static uint8_t amostrasCount[4] = {0, 0, 0, 0};
static uint8_t rejeicoesSeguidas[4] = {0, 0, 0, 0};

static bool fatorEscalaValido(float fator)
{
    return !isnan(fator) && !isinf(fator) && fabs(fator) > 0.0001f;
}

/////////////////////////////////////////////////////////////////////////////
// FILTRO: MEDIANA DE 3
/////////////////////////////////////////////////////////////////////////////
// Descarta o pico isolado e mantem o peso atualizado a cada pacote: o atraso
// e de apenas uma amostra, ao contrario de mediana longa ou media movel.

static float medianaDe3(float a, float b, float c)
{
    return max(min(a, b), min(max(a, b), c));
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

        // Pacote valido deste transmissor: renova a janela de conexao.
        // (0 e reservado para "nunca recebeu")
        uint32_t agora = millis();
        ultimoPacoteMs = (agora == 0) ? 1 : agora;

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

        // Tara automatica de partida: o valor bruto do transmissor inclui o
        // offset mecanico da plataforma, entao o primeiro pacote valido apos
        // o boot define o zero. Exige a plataforma vazia na energizacao.
        if (ready && sensorIndex >= 0 && sensorIndex < 4 && autoTaraPendente[sensorIndex])
        {
            autoTaraPendente[sensorIndex] = false;
            balanca.tare(valorLido, sensorIndex);
            sensorTarado[sensorIndex] = true;

            Serial.print("Tara automatica do sensor ");
            Serial.print(sensorIndex + 1);
            Serial.print(": ");
            Serial.println(valorLido, 3);
        }

        //////////////////////////////////////////////////////////////////////
        // VALIDACAO DO PACOTE
        //////////////////////////////////////////////////////////////////////
        // So e possivel avaliar o valor em kg depois que existe uma tara de
        // referencia. Antes disso o pacote passa direto.

        if (ready && sensorIndex >= 0 && sensorIndex < 4 && sensorTarado[sensorIndex])
        {
            float kgCandidato = balanca.get_units(rawValue) / 1000.0f;

            // Teto absoluto: acima da capacidade nao e peso, e corrupcao.
            if (isnan(kgCandidato) || isinf(kgCandidato) ||
                fabs(kgCandidato) > CAPACIDADE_MAX_KG)
            {
                Serial.print("REJEITADO (capacidade) sensor ");
                Serial.print(sensorIndex + 1);
                Serial.print(" RAW: ");
                Serial.println(rawValue, 3);
                return false;
            }

            // Coerencia entre pacotes: pega perda de digito na transmissao.
            if (amostrasCount[sensorIndex] > 0 &&
                fabs(kgCandidato - pesoKg) > SALTO_MAX_KG &&
                rejeicoesSeguidas[sensorIndex] < REJEICOES_ATE_RESSINCRONIZAR)
            {
                rejeicoesSeguidas[sensorIndex]++;
                Serial.print("REJEITADO (salto) sensor ");
                Serial.print(sensorIndex + 1);
                Serial.print(" RAW: ");
                Serial.println(rawValue, 3);
                return false;
            }

            rejeicoesSeguidas[sensorIndex] = 0;
        }

        //////////////////////////////////////////////////////////////////////
        // FILTRO
        //////////////////////////////////////////////////////////////////////

        float rawFiltrado = rawValue;

        if (sensorIndex >= 0 && sensorIndex < 4)
        {
            amostras[sensorIndex][2] = amostras[sensorIndex][1];
            amostras[sensorIndex][1] = amostras[sensorIndex][0];
            amostras[sensorIndex][0] = rawValue;

            if (amostrasCount[sensorIndex] < 3)
            {
                amostrasCount[sensorIndex]++;
            }

            if (amostrasCount[sensorIndex] == 3)
            {
                rawFiltrado = medianaDe3(amostras[sensorIndex][0],
                                         amostras[sensorIndex][1],
                                         amostras[sensorIndex][2]);
            }
        }

        pesoGramas = balanca.get_units(rawFiltrado);
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

/////////////////////////////////////////////////////////////////////////////
// MONITORAMENTO DE CONEXAO
/////////////////////////////////////////////////////////////////////////////

uint32_t SensorBalanca::tempoDesdeUltimoPacote()
{
    if (ultimoPacoteMs == 0)
    {
        return 0xFFFFFFFFul;
    }

    // Subtracao sem sinal: correta mesmo no overflow do millis() (~49 dias).
    return millis() - ultimoPacoteMs;
}

bool SensorBalanca::conectado()
{
    return tempoDesdeUltimoPacote() <= janelaConexaoMs;
}

void SensorBalanca::setJanelaConexao(uint32_t ms)
{
    if (ms > 0)
    {
        janelaConexaoMs = ms;
    }
}

uint32_t SensorBalanca::getJanelaConexao()
{
    return janelaConexaoMs;
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

    // Só conta como tara valida se havia uma leitura real do transmissor.
    // A tara chamada no boot, antes de qualquer pacote chegar, zera o offset
    // mas nao pode marcar o sensor como tarado.
    if (sensorIndex >= 0 && sensorIndex < 4)
    {
        sensorTarado[sensorIndex] = ready;

        // Descarta amostras anteriores ao novo zero.
        amostrasCount[sensorIndex] = 0;
        rejeicoesSeguidas[sensorIndex] = 0;
    }

    if (!ready)
    {
        Serial.print("AVISO: Sensor ");
        Serial.print(sensorIndex + 1);
        Serial.println(" tarado sem leitura valida. Refaca a tara com o transmissor conectado.");
    }

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

    // Sem tara o valor bruto inclui o peso da estrutura, o que entraria
    // direto no fator de escala.
    if (sensorIndex >= 0 && sensorIndex < 4 && !sensorTarado[sensorIndex])
    {
        Serial.print("ERRO: Sensor ");
        Serial.print(sensorIndex + 1);
        Serial.println(" precisa ser tarado antes de calibrar");
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
