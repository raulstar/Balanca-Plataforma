#include <Arduino.h>
#include <WiFi.h>
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

    // Configura WiFi usando WiFi (ESP32) – substitua pelos seus SSID/PASS
    const char* ssid = "raulstar";
    const char* password = "72989400";
    Serial.print("Conectando a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    // Aguarda conexão
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();
    Serial.print("Conectado, IP: ");
    Serial.println(WiFi.localIP());

    // Inicializa o servidor web definido em lib/WiFi_Server
    initWiFi();          // garante que o objeto server esteja configurado
    initWebServer();

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
