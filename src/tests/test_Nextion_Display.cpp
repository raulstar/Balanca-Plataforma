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
float ttotal = 0.0;

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
void handle_btsom();
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
    setNextionText("ttotal", String(ttotal, 1));

    setNextionText("ttara", String(gtara, 1));

    // tn agora é TEXT
    setNextionText("tn0", String(contadorRegistro));
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

        setNextionText("ttotal", String(ttotal, 1));

        setNextionText("ttara", String(gtara, 1));

        setNextionText("thora", ghora);

        setNextionText("tdata", gdata);

        setNextionText("gplaca", gplaca);

        // tn mostra contadorRegistro
        setNextionText("tn0", String(contadorRegistro));

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
        else if (command.indexOf("bsom") >= 0)
        {
            handle_bsom();
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
// EVENTO BSOM
// SOMA pesoAtual AO TOTAL
// ======================================================
void handle_bsom()
{
    Serial.println("Evento [bsom] recebido");

    // ==========================================
    // SE TOTAL FOR ZERO
    // ==========================================
    if (ttotal == 0.0)
    {
        ttotal = pesoAtual;
    }
    else
    {
        ttotal += pesoAtual;
    }

    // ==========================================
    // ATUALIZA DISPLAY
    // ==========================================
    setNextionText("ttotal", String(ttotal, 1));

    Serial.print("Novo ttotal = ");
    Serial.println(ttotal);
}

// ======================================================
// EVENTO BSALVAR
// ======================================================
// ======================================================
// EVENTO BSALVAR
// ======================================================
// ======================================================
// EVENTO BSALVAR
// ======================================================
// ======================================================
// EVENTO BSALVAR
// ======================================================
void handle_bsalvar()
{
    Serial.println("Evento [bsalvar] recebido");

    // ==================================================
    // VETORES COM NOMES DOS OBJETOS NEXTION
    // ==================================================

    String objTN[7] =
    {
        "tn0",
        "tn1",
        "tn2",
        "tn3",
        "tn4",
        "tn5",
        "tn6"
    };

    String objPLACA[7] =
    {
        "tplaca0",
        "tplaca1",
        "tplaca2",
        "tplaca3",
        "tplaca4",
        "tplaca5",
        "tplaca6"
    };

    String objDATA[7] =
    {
        "tdata0",
        "tdata1",
        "tdata2",
        "tdata3",
        "tdata4",
        "tdata5",
        "tdata6"
    };

    String objHORA[7] =
    {
        "thora0",
        "thora1",
        "thora2",
        "thora3",
        "thora4",
        "thora5",
        "thora6"
    };

    String objTOTAL[7] =
    {
        "ttotal0",
        "ttotal1",
        "ttotal2",
        "ttotal3",
        "ttotal4",
        "ttotal5",
        "ttotal6"
    };

    String objTARA[7] =
    {
        "ttara0",
        "ttara1",
        "ttara2",
        "ttara3",
        "ttara4",
        "ttara5",
        "ttara6"
    };

    // ==================================================
    // DESLOCA REGISTROS PARA BAIXO
    // ==================================================

    for (int i = 6; i > 0; i--)
    {
        for (int j = 0; j < 6; j++)
        {
            tabela[i][j] = tabela[i - 1][j];
        }
    }

    // ==================================================
    // NOVO REGISTRO NO TOPO
    // ==================================================

    tabela[0][0] = String(contadorRegistro);

    tabela[0][1] = gplaca;

    tabela[0][2] = gdata;

    tabela[0][3] = ghora;

    tabela[0][4] = String(ttotal, 1);

    tabela[0][5] = String(gtara, 1);

    // ==================================================
    // ENVIA TODA TABELA PARA O NEXTION
    // ==================================================

    for (int i = 0; i < 7; i++)
    {
        setNextionText(objTN[i], tabela[i][0]);

        setNextionText(objPLACA[i], tabela[i][1]);

        setNextionText(objDATA[i], tabela[i][2]);

        setNextionText(objHORA[i], tabela[i][3]);

        setNextionText(objTOTAL[i], tabela[i][4]);

        setNextionText(objTARA[i], tabela[i][5]);
    }

    // ==================================================
    // INCREMENTA CONTADOR
    // ==================================================

    contadorRegistro++;

    Serial.print("contadorRegistro = ");
    Serial.println(contadorRegistro);

    Serial.println("Tabela atualizada.");
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

    ttotal = 0.0;

    gtara = 0.0;

    // Limpa placa
    gplaca = "";

    // ==========================================
    // LIMPA DISPLAY
    // ==========================================
    setNextionText("tPeso", String(pesoAtual, 1));

    setNextionText("ttotal", String(ttotal, 1));

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