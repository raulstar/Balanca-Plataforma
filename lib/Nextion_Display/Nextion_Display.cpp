#include "Nextion_Display.hpp"

// ======================================================
// SERIAL NEXTION
// ======================================================
HardwareSerial NEXTION_SERIAL(1);

// ======================================================
// VARIÁVEIS GLOBAIS
// ======================================================
String placaVeiculo = "";
String dataRegistro = "";

String gdata = "";
String ghora = "";

String gplaca = "";

float pesoAtual = 0.0;
float ttotal = 0.0;
float gtara = 0.0;

String tabela[20][6];

int linhaAtual = 0;
int contadorRegistro = 1;

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
// INICIALIZA NEXTION
// ======================================================
void initNextion()
{
    NEXTION_SERIAL.begin(9600, SERIAL_8N1, NEXTION_RX, NEXTION_TX);

    Serial.println("Nextion iniciado");

    updateDisplay();
}

// ======================================================
// ATUALIZA DISPLAY
// ======================================================
void updateDisplay()
{
    setNextionText("thora", ghora);

    setNextionText("tdata", gdata);

    // setNextionText("gplaca", gplaca);

    setNextionText("tPeso", String(pesoAtual, 1));

    setNextionText("ttotal", String(ttotal, 1));

    setNextionText("ttara", String(gtara, 1));

    setNextionText("tn0", String(contadorRegistro));
}

// ======================================================
// PROCESSA COMANDOS
// ======================================================
void processNextionCommands()
{
    if (NEXTION_SERIAL.available())
    {
        String command = NEXTION_SERIAL.readStringUntil('\n');

        command.trim();

        //Serial.print("Comando Recebido: ");
        //Serial.println(command);

        if (command.indexOf("bsom") >= 0)
        {
            handle_bsom();
        }
        else if (command.indexOf("bzero") >= 0)
        {
            handle_bzero();
        }
        else if (command.indexOf("bsalvar") >= 0)
        {
            handle_bsalvar();
        }
        else if (command.indexOf("blimpar") >= 0)
        {
            handle_blimpar();
        }
        else if (command.indexOf("bcalib") >= 0)
        {
            handle_bcalib(command);
        }
        else if (command.indexOf("calib:") >= 0)
        {
            Serial.println("Evento [calib]");

            int pos = command.indexOf(':');

            if (pos > 0)
            {
                String valorStr = command.substring(pos + 1);

                pesoCalibracao1 = valorStr.toFloat();

                Serial.print("Peso de calibração 1: ");
                Serial.println(pesoCalibracao1, 2);
                calibrando1 = true;
            }
        }
        else if (command.startsWith("placa:"))
        {
            Serial.println("Evento [placa]");

            // pega tudo após "placa:"
            String valorStr = command.substring(6);

            // remove espaços extras no início/fim
            valorStr.trim();

            // salva na variável
            placaVeiculo = valorStr;

            Serial.print("Placa recebida: ");
            Serial.println(placaVeiculo);
            handle_bsalvar();
        }
    }
}

// ======================================================
// EVENTO BZERO
// ======================================================
void handle_bzero()
{
    Serial.println("Evento [bzero]");

    pesoAtual = 0.0;

    setNextionText("tPeso", String(pesoAtual, 1));
}

// ======================================================
// EVENTO BSOM
// ======================================================
void handle_bsom()
{
    Serial.println("Evento [bsom]");

    if (ttotal == 0.0)
    {
        ttotal = pesoAtual;
    }
    else
    {
        ttotal += pesoAtual;
    }

    setNextionText("ttotal", String(ttotal, 1));
}

// ======================================================
// EVENTO BSALVAR
// ======================================================
void handle_bsalvar()
{
    Serial.println("Evento [bsalvar]");

    String objTN[7] =
        {
            "tn0", "tn1", "tn2", "tn3", "tn4", "tn5", "tn6"};

    String objPLACA[7] =
        {
            "tplaca0", "tplaca1", "tplaca2", "tplaca3", "tplaca4", "tplaca5", "tplaca6"};

    String objDATA[7] =
        {
            "tdata0", "tdata1", "tdata2", "tdata3", "tdata4", "tdata5", "tdata6"};

    String objHORA[7] =
        {
            "thora0", "thora1", "thora2", "thora3", "thora4", "thora5", "thora6"};

    String objTOTAL[7] =
        {
            "ttotal0", "ttotal1", "ttotal2", "ttotal3", "ttotal4", "ttotal5", "ttotal6"};

    String objTARA[7] =
        {
            "ttara0", "ttara1", "ttara2", "ttara3", "ttara4", "ttara5", "ttara6"};

    for (int i = 6; i > 0; i--)
    {
        for (int j = 0; j < 6; j++)
        {
            tabela[i][j] = tabela[i - 1][j];
        }
    }

    tabela[0][0] = String(contadorRegistro);
    tabela[0][1] = gplaca;
    tabela[0][2] = gdata;
    tabela[0][3] = ghora;
    tabela[0][4] = String(ttotal, 1);
    tabela[0][5] = String(gtara, 1);

    for (int i = 0; i < 7; i++)
    {
        setNextionText(objTN[i], tabela[i][0]);

        setNextionText(objPLACA[i], tabela[i][1]);

        setNextionText(objDATA[i], tabela[i][2]);

        setNextionText(objHORA[i], tabela[i][3]);

        setNextionText(objTOTAL[i], tabela[i][4]);

        setNextionText(objTARA[i], tabela[i][5]);
    }

    contadorRegistro++;
}

// ======================================================
// EVENTO BLIMPAR
// ======================================================
void handle_blimpar()
{
    Serial.println("Evento [blimpar]");

    pesoAtual = 0.0;

    ttotal = 0.0;

    gtara = 0.0;

    gplaca = "";

    updateDisplay();
}

// ======================================================
// EVENTOS GENÉRICOS
// ======================================================
void handle_bgeneric(String cmd)
{
    Serial.print("Outro comando: ");
    Serial.println(cmd);
}

// ======================================================
// EVENTO BCALIB
// ======================================================
void handle_bcalib(String cmd)
{
    Serial.println("Evento [bcalib]");

    // Exemplo recebido:
    // bcalib:41A00000

    int pos = cmd.indexOf(':');

    if (pos > 0)
    {
        String hexValue = cmd.substring(pos + 1);

        uint32_t hexInt = strtoul(hexValue.c_str(), NULL, 16);

        float valor;

        memcpy(&valor, &hexInt, sizeof(valor));

        pesoCalibracao1 = valor;

        Serial.print("pesoCalibracao1 = ");
        Serial.println(pesoCalibracao1, 4);
    }
}