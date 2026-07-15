#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "WiFi_Server.hpp"
#include "../lib/WiFi_Server/PaginaHTML.h"

/**
 * Teste simples de conexão WiFi usando WiFiManager e do servidor web.
 * O servidor serve a página HTML definida em PaginaHTML.h na raiz "/".
 */

void setup() {
    // Inicializa a comunicação serial para depuração
    Serial.begin(115200);
    Serial.println("\n=== TESTE WIFI ===");

    // ---------------------------------------------------------------
    // Seleciona modo de operação Wi‑Fi.
    //   false – modo estação (conecta a rede existente)
    //   true  – modo Access Point (AP) – permite conexão direta ao
    //            controlador e acesso à página HTML.
    // ---------------------------------------------------------------
    bool useAP = false; // altere para false para usar modo estação
    setAPMode(useAP);

    // Inicializa Wi‑Fi (modo escolhido acima) e o servidor web.
    initWiFi();          // garante que o objeto server esteja configurado
    initWebServer();

    // Exibe informações de rede
    if (useAP) {
        Serial.print("AP ativo – SSID: ");
        Serial.println("Balanca_AP");
        Serial.print("IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.print("Conectado, IP: ");
        Serial.println(WiFi.localIP());
    }

    // Rota raiz que devolve a página HTML de exemplo
    server.on("/", [](){
        server.send(200, "text/html", pagina_html);
    });
}

void loop() {
    // Processa requisições HTTP
    handleWeb();
    delay(10);
}
