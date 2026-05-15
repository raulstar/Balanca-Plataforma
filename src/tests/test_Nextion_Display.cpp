#include <Arduino.h>

// Configuração da UART1
#define RXD1 16
#define TXD1 17
HardwareSerial NEXTION_SERIAL(1);

String placaVeiculo = "";
String dataRegistro = "";

// --- Novas Variáveis Globais ---
String gdata = "14/05/2026";
String ghora = "10:45"; // sem segundos
const String tplaca = "BRA2E19"; // somente leitura
String gtotal = "5000.0";
String gtara = "1200.0";

// Matriz 20x5
String tabela[20][6];
int linhaAtual = 0;
int contadorRegistro = 1;

// --- Protótipos das Funções ---
void handle_bsom();
void handle_bsalvar();
void handle_blimpar();
void handle_bgeneric(String cmd);
void handle_entrada_texto(String rawData);

// Função para atualizar textos (Objetos "t")
void setNextionText(String objName, String value) {
    NEXTION_SERIAL.print(objName + ".txt=\"" + value + "\"");
    NEXTION_SERIAL.write(0xff);
    NEXTION_SERIAL.write(0xff);
    NEXTION_SERIAL.write(0xff);
}

void setup() {
    Serial.begin(115200);
    // Nextion geralmente opera em 9600 ou 115200
    NEXTION_SERIAL.begin(9600, SERIAL_8N1, 25, 26); // RX, TX

    Serial.println("Monitor Serial pronto. Aguardando comandos do Nextion...");

    // Atualiza display na inicialização
    setNextionText("thora", ghora);
    setNextionText("gplaca", tplaca);
    setNextionText("tdata", gdata);
}

void loop() {
    // Exemplo de atualização de variáveis de texto ("t")
    static unsigned long tUpdate = 0;
    if (millis() - tUpdate > 5000) {
        setNextionText("tPeso", "125.4");
        setNextionText("ttotal", gtotal);
        setNextionText("ttara", gtara);

        // Atualiza objetos do display
        setNextionText("thora", ghora);
        setNextionText("gplaca", tplaca);
        setNextionText("tdata", gdata);

        tUpdate = millis();
    }

    // --- Processamento de Comandos "b" via printh ---
    if (NEXTION_SERIAL.available() > 0) {
        // Lemos a string enviada pelo printh
        String command = NEXTION_SERIAL.readString();
        command.trim(); // Remove espaços ou caracteres fantasmas

        if (command == "bsom") {
            handle_bsom();
        }
        else if (command == "bsalvar") {
            handle_bsalvar();
        }
        else if (command == "blimpar") {
            handle_blimpar();
        }
        else {
            handle_bgeneric(command);
        }
    }
}

// --- Implementação das Funções de cada Botão ---

void handle_bsom() {
    Serial.println("Comando Recebido: [bsom] -> Ativando sinal sonoro...");
    // Sua lógica de som aqui
}

void handle_bsalvar() {
    Serial.println("Comando Recebido: [bsalvar] -> Gravando dados na memória...");

    // Preenche matriz
    tabela[linhaAtual][0] = String(contadorRegistro);
    tabela[linhaAtual][1] = tplaca;
    tabela[linhaAtual][2] = gdata;
    tabela[linhaAtual][3] = ghora;
    tabela[linhaAtual][4] = gtotal;
    tabela[linhaAtual][5] = gtara;

    // Envia contador para display
    String sufixo = (linhaAtual == 0) ? "" : String(linhaAtual);

    // Atualiza número do registro
    setNextionText("tn" + sufixo, String(contadorRegistro));

    // Atualiza demais campos
    setNextionText("tplaca" + sufixo, tabela[linhaAtual][1]);
    setNextionText("tdata" + sufixo, tabela[linhaAtual][2]);
    setNextionText("thora" + sufixo, tabela[linhaAtual][3]);
    setNextionText("ttotal" + sufixo, tabela[linhaAtual][4]);
    setNextionText("ttara" + sufixo, tabela[linhaAtual][5]);

    Serial.println("Registro salvo na matriz e enviado ao display.");

    contadorRegistro++;
    linhaAtual++;

    if (linhaAtual >= 20) {
        linhaAtual = 0;
    }
}

void handle_blimpar() {
    Serial.println("Comando Recebido: [blimpar] -> Resetando campos da tela...");

    // Limpa somente os campos editáveis
    setNextionText("tPeso", "");
    setNextionText("ttotal", "");
    setNextionText("gplaca", "");

    // Mantém data e hora preservadas
    setNextionText("thora", ghora);
    setNextionText("tdata", gdata);
}

void handle_bgeneric(String cmd) {
    Serial.print("Outro botão pressionado: ");
    Serial.println(cmd);
}