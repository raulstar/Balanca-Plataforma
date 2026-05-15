#include <Arduino.h>

// ======================================================
// CONFIGURAÇÃO UART NEXTION
// ======================================================
#define NEXTION_RX 25
#define NEXTION_TX 26

HardwareSerial NEXTION_SERIAL(1);

// ======================================================
// VARIÁVEIS GLOBAIS
// ======================================================
String placaVeiculo = "";
String dataRegistro = "";

// ======================================================
// DATA / HORA
// ======================================================
String gdata = "14/05/2026";
String ghora = "10:45";

// ======================================================
// PLACA GLOBAL EDITÁVEL
// ======================================================
String gplaca = "BRA2E19";

// ======================================================
// VARIÁVEIS NUMÉRICAS
// ======================================================
float pesoAtual = 125.4;

// TOTAL inicia em ZERO
float gtotal = 0.0;

float gtara = 1200.0;

// ======================================================
// MATRIZ DE REGISTROS
// ======================================================
String tabela[20][6];

// ======================================================
// CONTADORES
// ======================================================
int linhaAtual = 0;
int contadorRegistro = 1;

// ======================================================
// PROTÓTIPOS
// ======================================================
void handle_bsom();
void handle_bzero();
void handle_tsom();
void handle_bsalvar();
void handle_blimpar();
void handle_bgeneric(String cmd);

void setNextionText(String objName, String value);

// ======================================================
// ENVIA TEXTO PARA NEXTION
// ======================================================
void setNextionText(String objName, String value)
{
    NEXTION_SERIAL.print(objName);
    NEXTION_SERIAL.print(".txt=\"");
    NEXTION_SERIAL.print(value);
    NEXTION_SERIAL.print("\"");

    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
}

// ======================================================
// SETUP
// ======================================================
void setup()
{
    Serial.begin(115200);

    // Inicializa UART do Nextion
    NEXTION_SERIAL.begin(9600, SERIAL_8N1, NEXTION_RX, NEXTION_TX);

    Serial.println("Sistema iniciado...");
    Serial.println("Aguardando comandos do Nextion...");

    // ==========================================
    // INICIALIZA DISPLAY
    // ==========================================
    setNextionText("thora", ghora);

    setNextionText("tdata", gdata);

    setNextionText("gplaca", gplaca);

    setNextionText("tPeso", String(pesoAtual, 1));

    // TOTAL inicia em ZERO
    setNextionText("ttotal", String(gtotal, 1));

    setNextionText("ttara", String(gtara, 1));

    // tn agora é TEXT
    setNextionText("tn", String(contadorRegistro));
}

// ======================================================
// LOOP PRINCIPAL
// ======================================================
void loop()
{
    // ==========================================
    // ATUALIZA DISPLAY PERIODICAMENTE
    // ==========================================
    static unsigned long tUpdate = 0;

    if (millis() - tUpdate > 2000)
    {
        setNextionText("tPeso", String(pesoAtual, 1));

        setNextionText("ttotal", String(gtotal, 1));

        setNextionText("ttara", String(gtara, 1));

        setNextionText("thora", ghora);

        setNextionText("tdata", gdata);

        setNextionText("gplaca", gplaca);

        // tn mostra contadorRegistro
        setNextionText("tn", String(contadorRegistro));

        tUpdate = millis();
    }

    // ==========================================
    // RECEBE COMANDOS NEXTION
    // ==========================================
    if (NEXTION_SERIAL.available())
    {
        String command = NEXTION_SERIAL.readStringUntil('\n');

        command.trim();

        Serial.print("Comando Recebido: ");
        Serial.println(command);

        // ======================================
        // EVENTOS
        // ======================================
        if (command.indexOf("bsom") >= 0)
        {
            handle_bsom();
        }
        else if (command.indexOf("bzero") >= 0)
        {
            handle_bzero();
        }
        else if (command.indexOf("tsom") >= 0)
        {
            handle_tsom();
        }
        else if (command.indexOf("bsalvar") >= 0)
        {
            handle_bsalvar();
        }
        else if (command.indexOf("blimpar") >= 0)
        {
            handle_blimpar();
        }
        else
        {
            handle_bgeneric(command);
        }
    }
}

// ======================================================
// EVENTO BSOM
// ======================================================
void handle_bsom()
{
    Serial.println("Evento [bsom] recebido");

    // Apenas exemplo de evento sonoro
    Serial.println("Som acionado.");
}

// ======================================================
// EVENTO BZERO
// ======================================================
void handle_bzero()
{
    Serial.println("Evento [bzero] recebido");

    // Zera peso atual
    pesoAtual = 0.0;

    // Atualiza display
    setNextionText("tPeso", String(pesoAtual, 1));

    Serial.println("pesoAtual zerado.");
}

// ======================================================
// EVENTO TSOM
// SOMA pesoAtual AO TOTAL
// ======================================================
void handle_tsom()
{
    Serial.println("Evento [tsom] recebido");

    // ==========================================
    // SE TOTAL FOR ZERO
    // ==========================================
    if (gtotal == 0.0)
    {
        gtotal = pesoAtual;
    }
    else
    {
        gtotal += pesoAtual;
    }

    // ==========================================
    // ATUALIZA DISPLAY
    // ==========================================
    setNextionText("ttotal", String(gtotal, 1));

    Serial.print("Novo gtotal = ");
    Serial.println(gtotal);
}

// ======================================================
// EVENTO BSALVAR
// ======================================================
void handle_bsalvar()
{
    Serial.println("Evento [bsalvar] recebido");

    // ==========================================
    // SALVA NA MATRIZ
    // ==========================================
    tabela[linhaAtual][0] = String(contadorRegistro);

    tabela[linhaAtual][1] = gplaca;

    tabela[linhaAtual][2] = gdata;

    tabela[linhaAtual][3] = ghora;

    tabela[linhaAtual][4] = String(gtotal, 1);

    tabela[linhaAtual][5] = String(gtara, 1);

    // ==========================================
    // SUFIXO DOS OBJETOS
    // ==========================================
    String idx = String(linhaAtual);

    // ==========================================
    // ENVIA PARA DISPLAY
    // ==========================================
    setNextionText("tn", String(contadorRegistro));

    setNextionText("tplaca" + idx, tabela[linhaAtual][1]);

    setNextionText("tdata" + idx, tabela[linhaAtual][2]);

    setNextionText("thora" + idx, tabela[linhaAtual][3]);

    setNextionText("ttotal" + idx, tabela[linhaAtual][4]);

    setNextionText("ttara" + idx, tabela[linhaAtual][5]);

    Serial.println("Registro salvo.");

    // ==========================================
    // INCREMENTA CONTADOR
    // ==========================================
    contadorRegistro++;

    Serial.print("contadorRegistro = ");
    Serial.println(contadorRegistro);

    // Atualiza tn novamente
    setNextionText("tn", String(contadorRegistro));

    // ==========================================
    // AVANÇA LINHA
    // ==========================================
    linhaAtual++;

    // Reinicia após linha 19
    if (linhaAtual >= 20)
    {
        linhaAtual = 0;
    }
}

// ======================================================
// EVENTO BLIMPAR
// ======================================================
void handle_blimpar()
{
    Serial.println("Evento [blimpar] recebido");

    // ==========================================
    // ZERA VARIÁVEIS
    // ==========================================
    pesoAtual = 0.0;

    gtotal = 0.0;

    gtara = 0.0;

    // Limpa placa
    gplaca = "";

    // ==========================================
    // LIMPA DISPLAY
    // ==========================================
    setNextionText("tPeso", String(pesoAtual, 1));

    setNextionText("ttotal", String(gtotal, 1));

    setNextionText("ttara", String(gtara, 1));

    setNextionText("gplaca", gplaca);

    // Mantém data/hora
    setNextionText("thora", ghora);

    setNextionText("tdata", gdata);

    Serial.println("Campos limpos.");
}

// ======================================================
// EVENTOS GENÉRICOS
// ======================================================
void handle_bgeneric(String cmd)
{
    Serial.print("Outro comando recebido: ");
    Serial.println(cmd);
}