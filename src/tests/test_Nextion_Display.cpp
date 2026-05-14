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
        setNextionText("ttotal", "5000.0");

        // Atualiza objetos do display
        setNextionText("thora", ghora);
        setNextionText("gplaca", tplaca);
        setNextionText("tdata", gdata);
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
    // Sua lógica de gravação (EEPROM/SD)
}

void handle_blimpar() {
    Serial.println("Comando Recebido: [blimpar] -> Resetando campos da tela...");

    // Mantém placa somente leitura
    setNextionText("gplaca", tplaca);
}

void handle_bgeneric(String cmd) {
    Serial.print("Outro botão pressionado: ");
    Serial.println(cmd);
}