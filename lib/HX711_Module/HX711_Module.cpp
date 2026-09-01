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

/////////////////////////////////////////////////////////////////////////////
// VALIDACAO DE PACOTE
/////////////////////////////////////////////////////////////////////////////
// A janela de variacao entre pacotes consecutivos foi removida. Ela nao
// funciona nesta aplicacao por dois motivos:
//
//  1. Frouxa demais para pegar corrupcao. Em campo "S1-23567" chegou como
//     "S1-23" -- uma variacao de 23544 counts, dentro dos 200000 da janela.
//     Passou, e a mediana teve de absorver o estrago.
//  2. Apertada demais para carga real. Um eixo subindo aplica milhares de
//     kg entre dois pacotes de ~1 s; para nao rejeitar isso a janela teria
//     de cobrir quase toda a faixa util, virando letra morta.
//
// Corrupcao de transmissao agora e barrada na origem pelo checksum do
// pacote, e o que resta e validado por faixa fisica absoluta.

// Exige o checksum "*XX" nos pacotes. Durante a migracao dos 4 transmissores
// mude para false: os antigos, sem checksum, voltam a ser aceitos (e a
// protecao contra pacote truncado deixa de valer para eles).
static const bool EXIGIR_CHECKSUM = true;

// FAIXA FISICA
// A janela de aceitacao relativa ao zero foi REMOVIDA. Ela era derivada do
// proprio fator de escala que deveria proteger, e isso fechava um laco:
//
//   fator errado -> janela estreita -> a carga de calibracao e rejeitada ->
//   a calibracao so pode gerar outro fator errado
//
// Em campo a S2 travou por completo desta forma. Com o fator de -4594 g/count
// gravado por uma calibracao ruim, a janela tinha 1360 counts; um ajuste
// mecanico moveu o zero em 8315 counts e TODO pacote passou a ser rejeitado.
// Como a rejeicao acontecia antes de renovar a janela de conexao, o sensor
// ainda aparecia como desconectado e a tara ficava inalcancavel. O unico
// escape era o comando "f[n]".
//
// Nao adianta redimensiona-la a partir de uma sensibilidade nominal: 5000 kg
// a ~528 counts/kg ja ocupam 2,6 milhoes dos 8,4 milhoes de counts do HX711.
// Somando sobrecarga e a folga necessaria para uma sensibilidade ate poucas
// vezes maior, a janela cobre praticamente todo o conversor. Ou seja, ela so
// e estreita quando o fator esta errado -- morde exatamente quando nao
// deveria, e nunca quando deveria.
//
// A protecao real ficou onde funciona: o checksum barra a corrupcao de
// transmissao na origem, o limite absoluto abaixo barra o que nao cabe no
// conversor, e fatorEscalaPlausivel() barra na entrada o fator absurdo que
// dava origem a tudo isto.

// Unico limite que sobrevive: o valor tem de caber nos 24 bits com sinal do
// HX711. Vale antes e depois da tara, e nao depende de calibracao alguma.
static const float LIMITE_ABSOLUTO_COUNTS = 8388607.0f;

// FAIXA PLAUSIVEL DO FATOR DE ESCALA, em gramas por count.
//
// Piso (0,05 g/count): o HX711 tem 24 bits. Cobrir 5000 kg gastando o
// conversor inteiro daria ~0,3 g/count, e nenhuma montagem real usa mais que
// isso. Abaixo de 0,05 o fator so pode ter vindo de erro de digitacao ou de
// uma calibracao com o peso informado na unidade errada.
//
// Teto (50 g/count): a divisao do controlador e de 5 kg. Com 50 g/count
// ainda restam 100 counts por divisao, o minimo para uma leitura solida.
// Acima disso o ruido de repouso passa a valer mais que uma divisao inteira.
// Foi o caso dos -4594 g/count aceitos em campo: 60 counts de deriva termica
// viraram 275 kg, e a plataforma "perdia a calibracao" parada com a carga em
// cima.
static const float FATOR_MIN_G_POR_COUNT = 0.05f;
static const float FATOR_MAX_G_POR_COUNT = 50.0f;

// ESTABILIDADE
// Numero de leituras comparadas e a faixa maxima aceita entre elas.
//
// A tolerancia e em counts do HX711, nao em kg, de proposito: assim a
// estabilidade pode ser avaliada ANTES da tara e nao depende do fator de
// escala. Essa independencia e essencial -- um fator errado na EEPROM
// (ja aconteceu com o S1) travaria a tara justamente quando ela e o unico
// caminho para consertar a calibracao.
//
// Dimensionada sobre o ruido medido em campo na plataforma S1, a
// ~528 counts/kg:
//   sem carga:  95 counts pico a pico (0,18 kg)
//   com carga:  45 counts pico a pico (0,09 kg)
//   deriva lenta: ~75 counts ao longo de 17 pacotes
//   pior janela de 5 amostras consecutivas observada: 74 counts
//
// 600 counts (~1,14 kg) dao ~8x de margem sobre esse pior caso e ainda
// ficam em 1/4 da divisao de 5 kg do controlador: apertado o bastante para
// um zero confiavel, folgado o bastante para nao travar com vibracao ou
// vento na plataforma carregada.
//
// A janela voltou a 5 amostras. Com 3, um par de pacotes ruins consecutivos
// bastava para a leitura "parecer" estavel e fixar um zero errado; a
// tolerancia mais folgada remove o motivo pelo qual ela tinha sido encurtada.
static const uint8_t ESTAB_AMOSTRAS = 5;
static const float ESTAB_TOLERANCIA_COUNTS = 600.0f;

// Tempo que a leitura precisa permanecer dentro da tolerancia.
// Com pacotes chegando a cada ~150-950 ms, 2000 ms garantem que a janela de
// 5 amostras seja de fato preenchida com dados novos antes de declarar
// estabilidade.
static const uint32_t ESTAB_TEMPO_MS = 2000;

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

// Leitura liquida minima para aceitar uma calibracao, em counts. Serve
// apenas para barrar o caso degenerado, em que o sinal e indistinguivel
// do ruido e o fator resultante seria arbitrario.
static const float CALIB_MIN_COUNTS = 50.0f;

static bool fatorEscalaValido(float fator)
{
    return !isnan(fator) && !isinf(fator) && fabs(fator) > 0.0001f;
}

// Alem de aritmeticamente utilizavel, o fator precisa ser fisicamente
// possivel para esta plataforma. O sinal e livre: carga entrando pode fazer
// o bruto subir ou descer, conforme a ligacao da celula.
static bool fatorEscalaPlausivel(float fator)
{
    if (!fatorEscalaValido(fator))
    {
        return false;
    }

    float magnitude = fabs(fator);

    return magnitude >= FATOR_MIN_G_POR_COUNT &&
           magnitude <= FATOR_MAX_G_POR_COUNT;
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

    float fatorCalculado = known_weight / leituraLiquida;

    Serial.print("Scale factor calculado:");
    Serial.println(fatorCalculado, 8);

    //////////////////////////////////////////////////////////////////////////
    // FATOR IMPLAUSIVEL
    //////////////////////////////////////////////////////////////////////////
    // Ultima barreira antes de gravar. CALIB_MIN_COUNTS olha so o tamanho do
    // sinal e nao basta: em campo, 850 kg produziram 185 counts, passaram
    // pelos 50 exigidos, e gravaram 4594 g/count. Com esse fator, 60 counts
    // de deriva termica viravam 275 kg e a plataforma parecia "perder a
    // calibracao" sozinha, com a carga parada em cima dela.
    //
    // O fator resultante e o que revela o absurdo, porque so ele confronta o
    // sinal medido com o peso que dizem ter sido aplicado. Recusado aqui, o
    // fator anterior e mantido intacto e nada e gravado na EEPROM.

    if (!fatorEscalaPlausivel(fatorCalculado))
    {
        Serial.print("ERRO: fator implausivel (");
        Serial.print(fatorCalculado, 4);
        Serial.println(" g/count). Calibracao recusada.");

        Serial.print("Esperado entre ");
        Serial.print(FATOR_MIN_G_POR_COUNT, 2);
        Serial.print(" e ");
        Serial.print(FATOR_MAX_G_POR_COUNT, 2);
        Serial.println(" g/count.");

        Serial.print("Foram apenas ");
        Serial.print(fabs(leituraLiquida), 0);
        Serial.print(" counts para ");
        Serial.print(known_weight / 1000.0f, 2);
        Serial.println(" kg: a carga nao esta chegando as celulas.");
        Serial.println("Verifique a mecanica da plataforma e a ligacao da caixa de juncao.");

        return false;
    }

    sensorScaleFactor = fatorCalculado;
    ::scale_factor = sensorScaleFactor;

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

float HX711::getOffset()
{
    return offset;
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

/////////////////////////////////////////////////////////////////////////////
// CHECKSUM
/////////////////////////////////////////////////////////////////////////////
// Pacote: <prefixo><valor>*<XOR em hex de 2 digitos>, ex. "S1-102796*3C".
// Devolve o indice do '*' quando o checksum confere, ou -1 se estiver
// ausente/invalido. O corpo a validar e tudo que vem antes do '*'.

static int validarChecksum(const String &s)
{
    int sep = s.lastIndexOf('*');

    // Sem checksum, ou sem os dois digitos hex depois dele.
    if (sep < 0 || s.length() != (unsigned int)(sep + 3))
    {
        return -1;
    }

    uint8_t calculado = 0;
    for (int i = 0; i < sep; ++i)
    {
        calculado ^= (uint8_t)s[i];
    }

    char *fim = nullptr;
    long recebido = strtol(s.c_str() + sep + 1, &fim, 16);

    if (fim == nullptr || *fim != '\0')
    {
        return -1;
    }

    return (calculado == (uint8_t)recebido) ? sep : -1;
}

bool SensorBalanca::processaString(String s)
{
    if (s.startsWith(prefixo))
    {
        //////////////////////////////////////////////////////////////////////
        // CHECKSUM
        //////////////////////////////////////////////////////////////////////
        // Barrado aqui, o pacote truncado nunca chega aos filtros. E o unico
        // ponto onde ele ainda e distinguivel de uma leitura legitima: depois
        // de virar numero, "S1-23" e tao plausivel quanto "S1-23567".

        int sep = validarChecksum(s);

        if (sep >= 0)
        {
            s = s.substring(0, sep);
        }
        else if (EXIGIR_CHECKSUM)
        {
            Serial.print("REJEITADO (checksum) ");
            Serial.println(s);
            return false;
        }

        //////////////////////////////////////////////////////////////////////
        // JANELA DE CONEXAO
        //////////////////////////////////////////////////////////////////////
        // Renovada aqui, e nao depois das validacoes de valor: um pacote com
        // checksum correto ja prova que o transmissor esta vivo, mesmo que o
        // valor que ele carrega venha a ser recusado adiante.
        //
        // Quando isto ficava depois das validacoes, uma sequencia de valores
        // recusados derrubava conectado(), que derruba estavel(), que bloqueia
        // tare() -- o sensor era declarado desconectado enquanto transmitia
        // sem parar, e a tara, unico caminho para sair da situacao, ficava
        // inalcancavel.
        // (0 e reservado para "nunca recebeu")

        uint32_t agora = millis();
        ultimoPacoteMs = (agora == 0) ? 1 : agora;

        float rawValue = atof(s.c_str() + prefixo.length());

        // Se o valor recebido raw for igual a zero, deve ser ignorado.
        if (rawValue == 0.0f)
        {
            return false;
        }

        //////////////////////////////////////////////////////////////////////
        // FAIXA FISICA
        //////////////////////////////////////////////////////////////////////
        // Resta apenas o limite do conversor. A janela relativa ao zero saiu
        // daqui: ver a nota no topo do arquivo.

        if (isnan(rawValue) || isinf(rawValue) ||
            fabs(rawValue) > LIMITE_ABSOLUTO_COUNTS)
        {
            Serial.print("REJEITADO (fora da faixa do HX711) ");
            Serial.print(prefixo);
            Serial.print(" RAW: ");
            Serial.println(rawValue, 3);
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

        // Em counts, para funcionar tambem antes da tara e independer do
        // fator de escala.
        atualizarEstabilidade(sensorIndex, rawFiltrado);

        //////////////////////////////////////////////////////////////////////
        // TARA AUTOMATICA DE PARTIDA
        //////////////////////////////////////////////////////////////////////
        // O valor bruto do transmissor inclui o offset mecanico da plataforma,
        // entao o zero e definido apos o boot. So acontece com a leitura
        // estavel, para nao fixar o zero durante uma oscilacao. Exige a
        // plataforma vazia na energizacao.
        //
        // Roda DEPOIS do filtro: antes, tarava com o valorFiltrado do pacote
        // anterior e conferia a estabilidade de uma amostra atrasada, entao o
        // zero gravado nao era o mesmo que o log anunciava.

        if (ready && sensorIndex >= 0 && sensorIndex < 4 &&
            autoTaraPendente[sensorIndex] && sensorEstavel[sensorIndex])
        {
            autoTaraPendente[sensorIndex] = false;
            balanca.tare(valorFiltrado, sensorIndex);
            sensorTarado[sensorIndex] = true;

            Serial.print("Tara automatica do sensor ");
            Serial.print(sensorIndex + 1);
            Serial.print(": ");
            Serial.println(valorFiltrado, 3);

            resetarFiltros(sensorIndex);
        }

        pesoGramas = balanca.get_units(valorFiltrado);
        pesoKg = pesoGramas / 1000.0f;

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

    // Sem isto o fator novo vive so em RAM: no proximo boot o setup()
    // recarrega fatorEscalaConhecido[] da EEPROM e restaura o fator antigo,
    // e a plataforma volta a pesar errado depois de ter sido calibrada.
    // Precisa ficar aqui, e nao no tratador do comando serial, porque a
    // calibracao agendada (executada quando a leitura estabiliza) nunca
    // passa por ele.
    salvarComEEPROM();

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

    // Avisa, mas nao recusa. O setup() passa por aqui aplicando o que veio da
    // EEPROM, e recusar deixaria a plataforma sem fator nenhum; o comando
    // "f[n]" tambem precisa continuar aceitando qualquer valor, para servir de
    // ajuste manual. O importante e que o operador saiba, no boot, quais
    // plataformas estao com fator que nao mede peso e precisam ser calibradas
    // de novo.
    if (fatorEscalaValido(scale) && !fatorEscalaPlausivel(scale))
    {
        Serial.print("AVISO: sensor ");
        Serial.print(sensorIndex + 1);
        Serial.print(" com fator implausivel (");
        Serial.print(scale, 4);
        Serial.println(" g/count). Recalibre antes de pesar.");
    }

    // Mantem fatorEscalaConhecido[] em dia com o fator em uso, para que o
    // comando "f[n] [fator]" possa ser gravado na EEPROM por quem o chamou.
    // A gravacao em si fica com o chamador: o setup() tambem passa por aqui
    // ao aplicar o que veio da EEPROM, e gravar aqui custaria quatro escritas
    // inuteis a cada boot.
    if (sensorIndex >= 0 && sensorIndex < 4 && fatorEscalaValido(scale))
    {
        fatorEscalaConhecido[sensorIndex] = scale;
    }
}

float SensorBalanca::getScale()
{
    return balanca.getScale();
}
