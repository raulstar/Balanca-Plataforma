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

// ---------------------------------------------------------------------------
// Global Wi‑Fi configuration / state variables (extern declarations)
// ---------------------------------------------------------------------------
extern const char *sta_ssid;      // SSID for station mode
extern const char *sta_password; // Password for station mode
extern const char *ap_ssid;      // SSID for Access Point mode
extern const char *ap_password; // Password for Access Point mode

extern bool g_apMode;            // true → AP mode, false → station mode
extern bool g_wifiConnected;    // connection status flag

extern int tentativas;           // current attempt counter
extern const int maxTentativas;  // maximum number of connection attempts

// ---------------------------------------------------------------------------
// Connection monitoring support
// ---------------------------------------------------------------------------
// Global flag indicating whether the device is currently connected to a Wi‑Fi
// network (station mode). In AP mode this flag is always true because the ESP32
// creates its own network.
extern bool g_wifiConnected;

// Checks the connection status and attempts to reconnect when the link is lost
// (only in station mode). Should be called periodically (e.g. from a task or
// the main loop).
void monitorWiFi();