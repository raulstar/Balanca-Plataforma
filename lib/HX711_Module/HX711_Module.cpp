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
bool sensorEstavel[4] = {false, false, false, false};

// A tara de partida nao pode ser feita no setup(): nenhum pacote chegou
// ainda. Fica pendente ate a primeira leitura valida de cada sensor.
static bool autoTaraPendente[4] = {true, true, true, true};

// Janela de variacao aceita entre dois pacotes consecutivos, em counts do
// HX711. Em counts a janela independe da tara e do fator de escala.
//
// Com ~852 counts/kg, 200000 counts equivalem a ~235 kg de variacao entre
// dois pacotes, o que cobre carga entrando na plataforma. Valores muito
// alem disso indicam pacote corrompido.
static const float JANELA_MAX_COUNTS = 200000.0f;

// Apos esta quantidade de rejeicoes seguidas, a referencia e considerada
// obsoleta e o proximo valor e aceito. Evita travar o sensor quando a
// leitura muda de patamar de forma legitima.
static const uint8_t REJEICOES_ATE_RESSINCRONIZAR = 3;

// Ultimo valor bruto aceito de cada sensor, referencia da janela.
static float ultimoRawAceito[4] = {0.0f, 0.0f, 0.0f, 0.0f};
static bool temReferencia[4] = {false, false, false, false};

// ESTABILIDADE
// Numero de leituras comparadas e a faixa maxima aceita entre elas.
// AJUSTE: uma tolerancia menor que o ruido do sensor faz a plataforma nunca
// estabilizar e trava tara e calibracao.
// A tolerancia e em counts do HX711, nao em kg: assim a estabilidade pode
// ser avaliada antes da tara e nao depende do fator de escala, que varia
// muito entre plataformas.
// AJUSTE: uma tolerancia menor que o ruido do transmissor faz a plataforma
// nunca estabilizar e trava a auto-tara, a tara e a calibracao.
// A janela curta acompanha melhor a deriva lenta da celula: com 5 amostras
// ela abrangia dois degraus do sinal e acusava instabilidade indevida.
//
// Dimensionada sobre a flutuacao medida com a plataforma sem carga:
// bruto oscilando ~175 counts pico a pico e deriva lenta que, apos a
// mediana de 3, produz ate 45 counts de amplitude em 3 amostras.
// 200 counts dao ~4x de margem sobre esse pior caso, sem deixar passar
// a rampa de convergencia do transmissor no boot (>700 counts).
static const uint8_t ESTAB_AMOSTRAS = 3;
static const float ESTAB_TOLERANCIA_COUNTS = 200.0f;

// Tempo que a leitura precisa permanecer dentro da tolerancia.
static const uint32_t ESTAB_TEMPO_MS = 1500;

// AGENDAMENTO
// Tara e calibracao pedidas com a leitura instavel ficam pendentes ate
// estabilizar. Passado este prazo a solicitacao e descartada.
static const uint32_t PENDENCIA_TIMEOUT_MS = 10000;

static const uint8_t PEND_NENHUM = 0;
static const uint8_t PEND_TARA = 1;
static const uint8_t PEND_CALIB = 2;

static uint8_t pendenciaTipo[4] = {PEND_NENHUM, PEND_NENHUM, PEND_NENHUM, PEND_NENHUM};
static float pendenciaPeso[4] = {0.0f, 0.0f, 0.0f, 0.0f};
static uint32_t pendenciaPrazoMs[4] = {0, 0, 0, 0};

static float estabBuffer[4][ESTAB_AMOSTRAS];
static uint8_t estabCount[4] = {0, 0, 0, 0};
static uint8_t estabIndice[4] = {0, 0, 0, 0};

// Instante em que a leitura entrou na tolerancia. Zero = fora dela.
static uint32_t estabDesdeMs[4] = {0, 0, 0, 0};

// Buffer de 3 amostras por sensor para a mediana.
static float amostras[4][3];
static uint8_t amostrasCount[4] = {0, 0, 0, 0};
static uint8_t rejeicoesSeguidas[4] = {0, 0, 0, 0};

// Leitura liquida minima para aceitar uma calibracao, em counts. Serve
// apenas para barrar o caso degenerado, em que o sinal e indistinguivel
// do ruido e o fator resultante seria arbitrario.
static const float CALIB_MIN_COUNTS = 50.0f;

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
// ESTABILIDADE
/////////////////////////////////////////////////////////////////////////////

// Descarta o historico de um sensor. Usado apos a tara, quando as leituras
// anteriores passam a se referir a outro zero.
static void resetarFiltros(int idx)
{
    if (idx < 0 || idx >= 4)
    {
        return;
    }

    amostrasCount[idx] = 0;
    rejeicoesSeguidas[idx] = 0;
    estabCount[idx] = 0;
    estabIndice[idx] = 0;
    estabDesdeMs[idx] = 0;
    sensorEstavel[idx] = false;
}

// Estavel = as ultimas ESTAB_AMOSTRAS leituras cabem dentro de
// ESTAB_TOLERANCIA_COUNTS e assim permaneceram por ESTAB_TEMPO_MS.
// Recebe o valor bruto filtrado, em counts.
static void atualizarEstabilidade(int idx, float counts)
{
    if (idx < 0 || idx >= 4)
    {
        return;
    }

    estabBuffer[idx][estabIndice[idx]] = counts;
    estabIndice[idx] = (estabIndice[idx] + 1) % ESTAB_AMOSTRAS;

    if (estabCount[idx] < ESTAB_AMOSTRAS)
    {
        estabCount[idx]++;
        sensorEstavel[idx] = false;
        return;
    }

    float minimo = estabBuffer[idx][0];
    float maximo = estabBuffer[idx][0];

    for (uint8_t i = 1; i < ESTAB_AMOSTRAS; ++i)
    {
        minimo = min(minimo, estabBuffer[idx][i]);
        maximo = max(maximo, estabBuffer[idx][i]);
    }

    if ((maximo - minimo) > ESTAB_TOLERANCIA_COUNTS)
    {
        // Movimento sobre a plataforma: reinicia a contagem de tempo.
        estabDesdeMs[idx] = 0;
        sensorEstavel[idx] = false;
        return;
    }

    uint32_t agora = millis();

    if (estabDesdeMs[idx] == 0)
    {
        estabDesdeMs[idx] = (agora == 0) ? 1 : agora;
    }

    sensorEstavel[idx] = (agora - estabDesdeMs[idx]) >= ESTAB_TEMPO_MS;
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

bool HX711::calibra(float leituraAtual, float known_weight)
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

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    // SINAL INSUFICIENTE
    //////////////////////////////////////////////////////////////////////////
    // Poucos counts para o peso aplicado significam celula ou ligacao com
    // problema. Gravar o fator assim produziria uma balanca que exibe
    // numeros sem medir nada.

    if (fabs(leituraLiquida) < CALIB_MIN_COUNTS)
    {
        Serial.print("ERRO: sinal insuficiente para calibrar. Apenas ");
        Serial.print(fabs(leituraLiquida), 0);
        Serial.print(" counts para ");
        Serial.print(known_weight / 1000.0f, 2);
        Serial.println(" kg.");

        Serial.print("Cada count valeria ");
        Serial.print((known_weight / 1000.0f) / fabs(leituraLiquida), 3);
        Serial.println(" kg. Verifique a celula e a ligacao do sensor.");

        return false;
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

    return true;
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
        // offset mecanico da plataforma, entao o zero e definido apos o boot.
        // Só acontece com a leitura estavel, para nao fixar o zero durante
        // uma oscilacao. Exige a plataforma vazia na energizacao.
        if (ready && sensorIndex >= 0 && sensorIndex < 4 &&
            autoTaraPendente[sensorIndex] && sensorEstavel[sensorIndex])
        {
            autoTaraPendente[sensorIndex] = false;
            balanca.tare(valorFiltrado, sensorIndex);
            sensorTarado[sensorIndex] = true;
            resetarFiltros(sensorIndex);

            Serial.print("Tara automatica do sensor ");
            Serial.print(sensorIndex + 1);
            Serial.print(": ");
            Serial.println(valorLido, 3);
        }

        //////////////////////////////////////////////////////////////////////
        // VALIDACAO DO PACOTE
        //////////////////////////////////////////////////////////////////////
        // Janela de variacao em counts, comparada ao ultimo valor aceito.
        // Em counts a validacao independe da tara e do fator de escala, entao
        // continua valendo antes da tara e apos mudanca eletrica no sensor.

        if (ready && sensorIndex >= 0 && sensorIndex < 4 &&
            temReferencia[sensorIndex])
        {
            float variacao = fabs(rawValue - ultimoRawAceito[sensorIndex]);

            if (isnan(rawValue) || isinf(rawValue) ||
                variacao > JANELA_MAX_COUNTS)
            {
                // Apos rejeicoes seguidas a referencia e considerada obsoleta
                // e o proximo valor e aceito. Sem isso uma mudanca legitima e
                // grande (recalibracao, ajuste eletrico) travaria o sensor.
                if (rejeicoesSeguidas[sensorIndex] < REJEICOES_ATE_RESSINCRONIZAR)
                {
                    rejeicoesSeguidas[sensorIndex]++;
                    Serial.print("REJEITADO (janela) sensor ");
                    Serial.print(sensorIndex + 1);
                    Serial.print(" RAW: ");
                    Serial.println(rawValue, 3);
                    return false;
                }

                Serial.print("RESSINCRONIZANDO sensor ");
                Serial.print(sensorIndex + 1);
                Serial.print(" RAW: ");
                Serial.println(rawValue, 3);
                resetarFiltros(sensorIndex);
            }
        }

        if (ready && sensorIndex >= 0 && sensorIndex < 4)
        {
            rejeicoesSeguidas[sensorIndex] = 0;
            ultimoRawAceito[sensorIndex] = rawValue;
            temReferencia[sensorIndex] = true;
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

        valorFiltrado = rawFiltrado;
        pesoGramas = balanca.get_units(rawFiltrado);
        pesoKg = pesoGramas / 1000.0f;

        // Em counts, para funcionar tambem antes da tara e independer do
        // fator de escala.
        atualizarEstabilidade(sensorIndex, rawFiltrado);

        // Executa tara ou calibracao agendada assim que houver estabilidade.
        processarPendencia();

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

/////////////////////////////////////////////////////////////////////////////
// AGENDAMENTO DE TARA E CALIBRACAO
/////////////////////////////////////////////////////////////////////////////

bool SensorBalanca::agendar(uint8_t tipo, float peso)
{
    if (sensorIndex < 0 || sensorIndex >= 4)
    {
        return false;
    }

    // Somente sensores ativos: sem transmissor nao ha leitura para esperar.
    if (!conectado())
    {
        Serial.print("ERRO: Sensor ");
        Serial.print(sensorIndex + 1);
        Serial.println(" desconectado. Solicitacao ignorada.");
        return false;
    }

    uint32_t prazo = millis() + PENDENCIA_TIMEOUT_MS;

    pendenciaTipo[sensorIndex] = tipo;
    pendenciaPeso[sensorIndex] = peso;
    pendenciaPrazoMs[sensorIndex] = (prazo == 0) ? 1 : prazo;

    return true;
}

bool SensorBalanca::pendente()
{
    if (sensorIndex < 0 || sensorIndex >= 4)
    {
        return false;
    }

    return pendenciaTipo[sensorIndex] != PEND_NENHUM;
}

void SensorBalanca::cancelarPendencia()
{
    if (sensorIndex >= 0 && sensorIndex < 4)
    {
        pendenciaTipo[sensorIndex] = PEND_NENHUM;
        pendenciaPrazoMs[sensorIndex] = 0;
    }
}

void SensorBalanca::processarPendencia()
{
    if (sensorIndex < 0 || sensorIndex >= 4 ||
        pendenciaTipo[sensorIndex] == PEND_NENHUM)
    {
        return;
    }

    // Subtracao sem sinal: correta mesmo no overflow do millis().
    if ((int32_t)(millis() - pendenciaPrazoMs[sensorIndex]) >= 0)
    {
        uint8_t tipo = pendenciaTipo[sensorIndex];
        cancelarPendencia();

        Serial.print("ERRO: Sensor ");
        Serial.print(sensorIndex + 1);
        Serial.print(tipo == PEND_TARA ? " zeragem" : " calibracao");
        Serial.println(" cancelada: leitura nao estabilizou a tempo.");
        return;
    }

    if (!estavel())
    {
        return;
    }

    // Limpa antes de executar: tare() e calibra() consultam a estabilidade
    // e, estando estaveis, executam sem reagendar.
    uint8_t tipo = pendenciaTipo[sensorIndex];
    float peso = pendenciaPeso[sensorIndex];
    cancelarPendencia();

    Serial.print("Sensor ");
    Serial.print(sensorIndex + 1);
    Serial.println(" estabilizou: executando solicitacao agendada.");

    if (tipo == PEND_TARA)
    {
        tare();
    }
    else
    {
        calibra(peso);
    }
}

bool SensorBalanca::estavel()
{
    if (sensorIndex < 0 || sensorIndex >= 4)
    {
        return false;
    }

    // Um transmissor mudo congela a ultima leitura, o que pareceria estavel.
    return sensorEstavel[sensorIndex] && conectado();
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

float SensorBalanca::getRawFiltrado()
{
    return valorFiltrado;
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
    // Sem leitura real do transmissor nao ha o que zerar.
    if (!ready)
    {
        Serial.print("ERRO: Sensor ");
        Serial.print(sensorIndex + 1);
        Serial.println(" sem leitura valida. Verifique o transmissor.");
        return false;
    }

    // Zerar durante oscilacao fixaria um zero errado: a solicitacao fica
    // agendada e e executada assim que a leitura estabilizar.
    if (!estavel())
    {
        if (agendar(PEND_TARA, 0.0f))
        {
            Serial.print("Sensor ");
            Serial.print(sensorIndex + 1);
            Serial.println(" instavel: zeragem agendada para quando estabilizar.");
        }
        return false;
    }

    // Usa o valor filtrado, nao o ultimo pacote: fixar o zero sobre uma
    // amostra unica embutiria o ruido daquele instante no offset.
    balanca.tare(valorFiltrado, sensorIndex);

    if (sensorIndex >= 0 && sensorIndex < 4)
    {
        sensorTarado[sensorIndex] = true;

        // Descarta amostras anteriores ao novo zero.
        resetarFiltros(sensorIndex);
    }

    // Forçar recalculamento imediato após definir novo offset.
    // Usa a mesma origem do offset, senao a diferenca entre o valor filtrado
    // e o ultimo pacote aparece como peso residual logo apos zerar.
    pesoGramas = balanca.get_units(valorFiltrado);
    pesoKg = pesoGramas / 1000.0f;

    Serial.println("--------------------------------");
    Serial.println("BALANCA ZERADA");
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

    // Calibrar com a leitura oscilando gravaria um fator de escala errado:
    // a solicitacao fica agendada ate estabilizar.
    if (!estavel())
    {
        if (agendar(PEND_CALIB, pesoConhecido))
        {
            Serial.print("Sensor ");
            Serial.print(sensorIndex + 1);
            Serial.print(" instavel: calibracao com ");
            Serial.print(pesoConhecido);
            Serial.println(" g agendada para quando estabilizar.");
        }
        return false;
    }

    // Pelo mesmo motivo da tara: o fator sai do valor filtrado.
    // Recusada, o fator anterior e mantido e nada deve ser anunciado.
    if (!balanca.calibra(valorFiltrado, pesoConhecido))
    {
        return false;
    }

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
