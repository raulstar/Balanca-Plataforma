#pragma once

#include <Arduino.h>

/////////////////////////////////////////////////////////////////////////////
// CLASSE HX711
/////////////////////////////////////////////////////////////////////////////

class HX711
{
private:

    float offset;
    float sensorKnownWeight;
    int sensorId;
    float sensorScaleFactor;
    //static const float defaultKnownWeight[4];
    //static const float defaultScaleFactor[4];

public:

    HX711();
    HX711(int sensorSlot);

    void tare(float leituraAtual, int sensorSlot);

    float get_units(float leituraAtual);

    // Retorna false se a calibracao foi recusada e o fator anterior mantido.
    bool calibra(float leituraAtual, float known_weight);

    void setScale(float scale);
    float getScale();

    // Zero atual, em counts. A validacao de faixa fisica do pacote precisa
    // dele para saber onde comeca a plataforma.
    float getOffset();
};

extern int sensorIndex[4];
extern float novoFator[4];
extern float scale_factor;

// Indica se cada sensor ja recebeu uma tara valida desde o boot.
// Vive apenas em RAM: e sempre falso apos reiniciar.
extern bool sensorTarado[4];

// Indica se a leitura de cada sensor esta estavel (parada dentro da
// tolerancia pelo tempo minimo). Tara e calibracao exigem estabilidade.
extern bool sensorEstavel[4];

/////////////////////////////////////////////////////////////////////////////
// CLASSE SENSOR BALANCA
/////////////////////////////////////////////////////////////////////////////

class SensorBalanca
{
private:

    //////////////////////////////////////////////////////////////////////////
    // SERIAL
    //////////////////////////////////////////////////////////////////////////

    HardwareSerial *serial;
    String prefixo;

    //////////////////////////////////////////////////////////////////////////
    // BUFFER RX
    //////////////////////////////////////////////////////////////////////////

    char rxBuffer[32];

    uint8_t rxIndex = 0;

    //////////////////////////////////////////////////////////////////////////
    // BALANÇA
    //////////////////////////////////////////////////////////////////////////

    HX711 balanca;

    //////////////////////////////////////////////////////////////////////////
    // DADOS
    //////////////////////////////////////////////////////////////////////////

    float valorLido = 0.0f;

    float valorFiltrado = 0.0f;

    float pesoGramas = 0.0f;

    float pesoKg = 0.0f;
    int sensorIndex = 0;

    //////////////////////////////////////////////////////////////////////////
    // STATUS
    //////////////////////////////////////////////////////////////////////////

    bool ready = false;

    //////////////////////////////////////////////////////////////////////////
    // MONITORAMENTO DE CONEXAO
    //////////////////////////////////////////////////////////////////////////

    // Marca o instante do ultimo pacote valido recebido deste transmissor.
    // Zero significa que nenhum pacote chegou desde o boot.
    uint32_t ultimoPacoteMs = 0;

    // Registra uma solicitacao de tara ou calibracao para quando a leitura
    // estabilizar. Retorna false se o sensor nao estiver ativo.
    bool agendar(uint8_t tipo, float peso);

    // Janela de tempo (ms) usada para decidir conectado/desconectado.
    // Base: no log, S2 chega em todo ciclo e S1 a cada ~5-6 ciclos.
    // 2000 ms cobre com folga (~3x) o maior intervalo observado do S1.
    uint32_t janelaConexaoMs = 2000;

public:

    //////////////////////////////////////////////////////////////////////////
    // CONSTRUTOR
    //////////////////////////////////////////////////////////////////////////

    SensorBalanca(HardwareSerial &porta, String prefixoSensor, int sensorSlot);

    //////////////////////////////////////////////////////////////////////////
    // LEITURA
    //////////////////////////////////////////////////////////////////////////

    bool leitura();

    bool processaString(String s);

    //////////////////////////////////////////////////////////////////////////
    // STATUS
    //////////////////////////////////////////////////////////////////////////

    bool isReady();

    // Retorna 1 (true) se o transmissor enviou algum dado valido dentro da
    // janela de tempo, 0 (false) se ficou mudo por mais que a janela.
    bool conectado();

    // Retorna true quando as ultimas leituras ficaram dentro da tolerancia
    // pelo tempo minimo exigido. Detecta movimento sobre a plataforma.
    bool estavel();

    //////////////////////////////////////////////////////////////////////////
    // AGENDAMENTO
    //////////////////////////////////////////////////////////////////////////
    // Tara e calibracao pedidas com a leitura instavel ficam pendentes e sao
    // executadas assim que ela estabilizar, ou descartadas apos o prazo.

    // Ha solicitacao aguardando estabilidade.
    bool pendente();

    // Descarta a solicitacao pendente.
    void cancelarPendencia();

    // Verifica prazo e estabilidade, executando a solicitacao quando possivel.
    // Chamada automaticamente a cada leitura recebida.
    void processarPendencia();

    // Tempo (ms) desde o ultimo pacote valido. Retorna 0xFFFFFFFF se nunca
    // recebeu nada.
    uint32_t tempoDesdeUltimoPacote();

    void setJanelaConexao(uint32_t ms);
    uint32_t getJanelaConexao();

    //////////////////////////////////////////////////////////////////////////
    // DADOS
    //////////////////////////////////////////////////////////////////////////

    float getRaw();

    // Valor bruto apos a mediana: e este, e nao getRaw(), que origina o peso.
    float getRawFiltrado();

    float getKg();

    float getGramas();

    //////////////////////////////////////////////////////////////////////////
    // TARA
    //////////////////////////////////////////////////////////////////////////

    bool tare();

    //////////////////////////////////////////////////////////////////////////
    // CALIBRAÇÃO
    //////////////////////////////////////////////////////////////////////////

    bool calibra(float pesoConhecido);

    void setScale(float scale);
    float getScale();
};
