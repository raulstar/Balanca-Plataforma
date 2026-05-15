#include <Arduino.h>

// ======================================================
// CONFIGURAÇÃO UART NEXTION
// ======================================================
#define RXD1 16
#define TXD1 17

HardwareSerial NEXTION_SERIAL(1);

// ======================================================
// VARIÁVEIS GLOBAIS
// ======================================================
String placaVeiculo = "";
String dataRegistro = "";

// Dados simulados
String gdata  = "14/05/2026";
String ghora  = "10:45";
String gtotal = "5000.0";
String gtara  = "1200.0";

const String tplaca = "BRA2E19";

// ======================================================
// CONTADORES
// ======================================================
int contadorRegistro = 7;
int contadorTN = 0;
int linhaAtual = 0;

// ======================================================
// MATRIZ DE REGISTROS
// ======================================================
String tabela[20][6];

// ======================================================
// PROTÓTIPOS
// ======================================================
void handle_bsom();
void handle_bsalvar();
void handle_blimpar();
void handle_bgeneric(String cmd);

void setNextionText(String objName, String value);
void setNextionValue(String objName, int value);

// ======================================================
// ENVIO DE TEXTO PARA NEXTION
// ======================================================
void setNextionText(String objName, String value)
{
    NEXTION_SERIAL.print(objName + ".txt=\"" + value + "\"");

    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
}

// ======================================================
// ENVIO DE VALOR NUMÉRICO PARA NEXTION
// ======================================================
void setNextionValue(String objName, int value)
{
    NEXTION_SERIAL.print(objName + ".val=" + String(value));

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

    // Inicializa UART Nextion
    NEXTION_SERIAL.begin(9600, SERIAL_8N1, 25, 26);

    Serial.println("Sistema iniciado...");
    Serial.println("Aguardando comandos do Nextion...");

    // ==========================================
    // INICIALIZA DISPLAY
    // ==========================================
    setNextionText("thora", ghora);
    setNextionText("gplaca", tplaca);
    setNextionText("tdata", gdata);

    // Inicializa objeto NUMBER tn
    setNextionValue("tn", contadorTN);

    // Inicializa pesos
    setNextionText("ttotal", gtotal);
    setNextionText("ttara", gtara);
}

// ======================================================
// LOOP PRINCIPAL
// ======================================================
void loop()
{
    // ==========================================
    // ATUALIZAÇÃO PERIÓDICA DISPLAY
    // ==========================================
    static unsigned long tUpdate = 0;

    if (millis() - tUpdate > 5000)
    {
        setNextionText("tPeso", "125.4");

        setNextionText("ttotal", gtotal);
        setNextionText("ttara", gtara);

        setNextionText("thora", ghora);
        setNextionText("gplaca", tplaca);
        setNextionText("tdata", gdata);

        tUpdate = millis();
    }

    // ==========================================
    // RECEBIMENTO DE COMANDOS
    // ==========================================
    if (NEXTION_SERIAL.available())
    {
        String command = NEXTION_SERIAL.readString();

        command.trim();

        Serial.print("Comando recebido: ");
        Serial.println(command);

        // ======================================
        // EVENTOS
        // ======================================
        if (command == "bsom")
        {
            handle_bsom();
        }
        else if (command == "bsalvar")
        {
            handle_bsalvar();
        }
        else if (command == "blimpar")
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
    Serial.println("Evento BSOM recebido");

    // Incrementa contador
    contadorTN++;

    // Atualiza objeto NUMBER no Nextion
    setNextionValue("tn", contadorTN);

    Serial.print("TN atualizado para: ");
    Serial.println(contadorTN);

    // Aqui pode adicionar buzzer futuramente
}

// ======================================================
// EVENTO BSALVAR
// ======================================================
void handle_bsalvar()
{
    Serial.println("Salvando registro...");

    // ==========================================
    // SALVA MATRIZ
    // ==========================================
    tabela[linhaAtual][0] = String(contadorRegistro);
    tabela[linhaAtual][1] = tplaca;
    tabela[linhaAtual][2] = gdata;
    tabela[linhaAtual][3] = ghora;
    tabela[linhaAtual][4] = gtotal;
    tabela[linhaAtual][5] = gtara;

    // ==========================================
    // SUFIXO DOS CAMPOS
    // ==========================================
    String sufixo = (linhaAtual == 0) ? "" : String(linhaAtual);

    // ==========================================
    // ENVIA PARA DISPLAY
    // ==========================================

    // tn é NUMBER
    setNextionValue("tn" + sufixo, contadorRegistro);

    // textos
    setNextionText("tplaca" + sufixo, tabela[linhaAtual][1]);
    setNextionText("tdata"  + sufixo, tabela[linhaAtual][2]);
    setNextionText("thora"  + sufixo, tabela[linhaAtual][3]);
    setNextionText("ttotal" + sufixo, tabela[linhaAtual][4]);
    setNextionText("ttara"  + sufixo, tabela[linhaAtual][5]);

    Serial.println("Registro salvo e enviado ao display.");

    // ==========================================
    // AVANÇA LINHA
    // ==========================================
    contadorRegistro++;
    linhaAtual++;

    // Reinicia ao chegar em 20
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
    Serial.println("Limpando campos do display...");

    // Limpa campos
    setNextionText("tPeso", "");
    setNextionText("ttotal", "");
    setNextionText("gplaca", "");

    // Mantém data/hora
    setNextionText("thora", ghora);
    setNextionText("tdata", gdata);

    // Zera contador TN
    contadorTN = 0;

    // Atualiza NUMBER tn
    setNextionValue("tn", contadorTN);
}

// ======================================================
// OUTROS BOTÕES
// ======================================================
void handle_bgeneric(String cmd)
{
    Serial.print("Outro botão pressionado: ");
    Serial.println(cmd);
}