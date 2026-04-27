#pragma once
#include <WebServer.h>

extern WebServer server;

void initWiFi();
void initWebServer();
void handleWeb();