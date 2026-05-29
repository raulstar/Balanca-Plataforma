#pragma once
#include <WebServer.h>

extern WebServer server;

void initWiFi();
void initWebServer();
void handleWeb();

// ---------------------------------------------------------------------------
// AP mode support
// ---------------------------------------------------------------------------
// Global flag indicating whether the device should operate in Access Point (AP)
// mode instead of connecting to an existing Wi‑Fi network (station mode).
extern bool g_apMode;

// Set the AP mode flag. Call before initWiFi().
void setAPMode(bool enable);