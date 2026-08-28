#include "WiFi_Server.hpp"

// Implementação padrão fraca para permitir compilar testes isolados que não
// linkam src/main.cpp. No firmware principal, a implementação forte em main.cpp
// sobrescreve esta função.
void __attribute__((weak)) tareAllSensors()
{
}
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "PaginaHTML.h"
#include "Nextion_Display.hpp"

// Importação de variáveis globais do Nextion_Display.cpp
extern String tabela[20][11];
extern float ttara;
extern String thora;
extern String tdata;
extern String tbateria;
extern String last_thora;
extern String last_tdata;
extern volatile bool imprimir;

WebServer server(80);

// Default credentials for station mode (client)
String sta_ssid = "Revlo_Claro";
String sta_password = "Revlo@2025";

// Credentials for AP mode – can be changed via setAPMode if needed
String ap_ssid = "Balanca_AP";
String ap_password = "12345678";
String dnsAddress = "http://balanca.local";

// Global flag indicating AP mode (default false – station mode)
bool g_apMode = false;

// Global flag indicating connection status (true when STA is connected).
// In AP mode this flag is always true because the ESP creates its own AP.
bool g_wifiConnected = true;

static String getMDNSHostFromAddress()
{
    String host = dnsAddress;
    host.replace("http://", "");
    host.replace("https://", "");

    int slashIndex = host.indexOf('/');
    if (slashIndex >= 0) {
        host = host.substring(0, slashIndex);
    }

    if (host.endsWith(".local")) {
        host = host.substring(0, host.length() - 6);
    }

    return host;
}

void setAPMode(bool enable) {
    g_apMode = enable;
}
int tentativas = 2;
const int maxTentativas = 10;

bool ledState = false;

void handleRoot()
{
    String html = FPSTR(pagina_html);
    html.replace("{{THORA}}", last_thora.length() > 0 ? last_thora : "--");
    html.replace("{{TDATA}}", last_tdata.length() > 0 ? last_tdata : "--");
    html.replace("{{TBATERIA}}", tbateria.length() > 0 ? tbateria : "--");
    server.send(200, "text/html", html);
}

void handleStatus()
{
    String json = "{";
    json += "\"wifi\":\"Conectado\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

void handleCmd()
{
    String cmd = server.arg("c");

    if (cmd == "led_on")
    {
        digitalWrite(2, HIGH);
        ledState = true;
    }
    else if (cmd == "led_off")
    {
        digitalWrite(2, LOW);
        ledState = false;
    }

    server.send(200, "text/plain", "OK");
}

void handlePeso()
{
    float peso = random(100, 500); // simulação
    server.send(200, "text/plain", String(peso));
}

void initWiFi()
{
    if (g_apMode) {
        // Start Access Point mode
        Serial.println("Iniciando modo Access Point...");
        WiFi.mode(WIFI_AP);
        bool result = WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
        if (result) {
            Serial.println("AP iniciado com sucesso.");
            Serial.print("SSID: ");
            Serial.println(ap_ssid);
            Serial.print("IP: ");
            Serial.println(WiFi.softAPIP());
        } else {
            Serial.println("Falha ao iniciar AP.");
        }
        // MDNS can also be started for AP if desired
        String mdnsHost = getMDNSHostFromAddress();
        if (MDNS.begin(mdnsHost.c_str())) {
            MDNS.addService("http", "tcp", 80);
            Serial.println("MDNS iniciado: " + dnsAddress);
        }
        // In AP mode we consider ourselves always "connected"
        g_wifiConnected = true;
    } else {
        // Station mode – connect to existing Wi‑Fi network
        WiFi.mode(WIFI_STA);
        WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
        while (WiFi.status() != WL_CONNECTED && tentativas < maxTentativas) {
            delay(500);
            Serial.print('.');
            tentativas++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi Conectado!");
            Serial.println("IP: " + WiFi.localIP().toString());
            String mdnsHost = getMDNSHostFromAddress();
            if (MDNS.begin(mdnsHost.c_str())) {
                Serial.println("MDNS iniciado: " + dnsAddress);
                MDNS.addService("http", "tcp", 80);
            } else {
                Serial.println("Erro ao iniciar MDNS");
            }
            g_wifiConnected = true;
        } else {
            Serial.println("\nFalha ao conectar no WiFi.");
            g_wifiConnected = false;
        }
    }
    // ======================================================
}

// ---------------------------------------------------------------------------
// monitorWiFi – called periodically to ensure the device stays connected.
// In AP mode the function does nothing because the ESP is always reachable.
// In station mode it checks the connection status and attempts a reconnection
// using the stored credentials when the link is lost.
// ---------------------------------------------------------------------------
void monitorWiFi()
{
    if (g_apMode) {
        // No monitoring needed in AP mode.
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (!g_wifiConnected) {
            Serial.println("WiFi reconectado.");
        }
        g_wifiConnected = true;
        return;
    }

    // Not connected – attempt reconnection
    Serial.println("WiFi desconectado – tentando reconectar...");
    g_wifiConnected = false;
    // Simple reconnection strategy: call WiFi.reconnect() which uses the
    // credentials supplied in the previous WiFi.begin(). If that fails, we
    // fall back to a full WiFi.begin() after a short delay.
    WiFi.reconnect();
    unsigned long start = millis();
    while (millis() - start < 5000 && WiFi.status() != WL_CONNECTED) {
        delay(200);
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Reconnection successful.");
        String mdnsHost = getMDNSHostFromAddress();
        if (MDNS.begin(mdnsHost.c_str())) {
            MDNS.addService("http", "tcp", 80);
            Serial.println("MDNS reiniciado: " + dnsAddress);
        }
        g_wifiConnected = true;
    } else {
        Serial.println("Reconnection failed – will retry on next monitor call.");
    }
}

void handleSalvar()
{
    if (server.hasArg("placa"))
    {
        tplaca = server.arg("placa");
        tplaca.toUpperCase();
        placaVeiculo = tplaca;
    }
    handle_bsalvar();
    server.send(200, "text/plain", "Registro salvo");
}

void handleLimpar()
{
    handle_blimpar();
    server.send(200, "text/plain", "Dados limpos");
}

void initWebServer()
{

    //pinMode(2, OUTPUT);

    server.on("/", handleRoot);
    server.on("/status", handleStatus);
    server.on("/cmd", handleCmd);
    server.on("/peso", handlePeso);
    server.on("/dados", handleDados);
    server.on("/zero", handleZero);
    server.on("/salvar", handleSalvar);
    server.on("/limpar", handleLimpar);
    server.on("/imprimir", handleImprimir);

    server.begin();
}

void handleWeb()
{
    server.handleClient();
}

// Handler for "/imprimir" endpoint - sinaliza o inicio da impressao.
// A flag global 'imprimir' e monitorada pelo firmware, do mesmo modo
// que o comando "imprimi" recebido do display Nextion.
void handleImprimir()
{
    imprimir = true;
    Serial.println("Evento [imprimir] via web");
    server.send(200, "text/plain", "Impressao solicitada");
}

// Handler for "/zero" endpoint – performs tare (zero) of all sensors.
// Returns a simple plain‑text confirmation.
void handleZero()
{
    // Reuse existing tare wrapper to zero all sensors.
    tareAllSensors();
    server.send(200, "text/plain", "Sensors zeroed");
}

// Handler for "/dados" endpoint – returns current weight data.
// For now we provide a minimal JSON payload with the current total weight.
void handleDados()
{
    float pesoAtualSeguro = (!isnan(pesoAtual) && !isinf(pesoAtual)) ? pesoAtual : 0.0f;
    float ttotalSeguro = (!isnan(ttotal) && !isinf(ttotal)) ? ttotal : 0.0f;

    String json = "{\"pesoAtual\":" + String(pesoAtualSeguro, 2) +
                  ",\"peixo\":" + String(peixo) +
                  ",\"pesoAcumulado\":" + String(ttotalSeguro, 2) +
                  ",\"ttotal\":" + String(ttotalSeguro, 2) +
                  ",\"contadorRegistro\":" + String(contadorRegistro) +
                  ",\"tbateria\":\"" + tbateria + "\"" +
                  ",\"thora\":\"" + thora + "\"" +
                  ",\"tdata\":\"" + tdata + "\"" +
                  ",\"last_thora\":\"" + last_thora + "\"" +
                  ",\"last_tdata\":\"" + last_tdata + "\"" +
                  ",\"tpeso1\":\"" + tpeso1 + "\"" +
                  ",\"tpeso2\":\"" + tpeso2 + "\"";

    // Adiciona o nó 'historico' ao JSON extraindo os dados da matriz 'tabela'
    json += ",\"historico\":[";
    
    // Percorre as 20 posicoes da tabela. Como a gravacao e circular,
    // contadorRegistro e apenas o proximo indice de escrita e nao serve
    // como quantidade: publica toda linha que tenha conteudo gravado.
    const int maxRegistros = 20;
    bool primeiroRegistro = true;

    for (int i = 0; i < maxRegistros; i++)
    {
        if (tabela[i][0].length() == 0)
        {
            continue; // linha ainda nao utilizada
        }

        if (!primeiroRegistro) json += ","; // Separa os objetos com vírgula
        primeiroRegistro = false;

        json += "{";
        json += "\"n\":\"" + tabela[i][0] + "\",";
        json += "\"placa\":\"" + tabela[i][1] + "\",";
        json += "\"data\":\"" + tabela[i][2] + " " + tabela[i][3] + "\","; // Junta data e hora
        json += "\"total\":\"" + tabela[i][10] + "\","; // O ttotal está gravado no índice 10
        json += "\"tara\":\"" + String(ttara, 2) + "\"";  // A tara é global 
        json += "}";
    }
    
    json += "]";
    json += "}";

    server.send(200, "application/json", json);
}