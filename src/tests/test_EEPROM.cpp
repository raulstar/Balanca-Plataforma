#include <Arduino.h>
#include "EEPROM_Module.hpp"

float pesoAtual = 0.0f;
float ttotal = 0.0f;
String sta_ssid = "";
String sta_password = "";
String ap_ssid = "";
String ap_password = "";
String tplaca = "";
bool g_wifiConnected = false;
bool g_apMode = false;
String tabela[20][11];

static const float expectedPesoConhecido[4] = {11111.1f, 22222.2f, 33333.3f, 44444.4f};
static const float expectedFatorEscalaConhecido[4] = {-1111.1f, -2222.2f, -3333.3f, -4444.4f};

static bool floatEquals(float a, float b, float epsilon = 0.01f)
{
    return fabs(a - b) <= epsilon;
}

static void printFloatArray(const char *label, const float values[4])
{
    Serial.print(label);
    Serial.print(" = [");
    for (int i = 0; i < 4; ++i)
    {
        Serial.print(values[i], 2);
        if (i < 3)
        {
            Serial.print(", ");
        }
    }
    Serial.println("]");
}

static bool validateFloatArray(const char *label, const float values[4], const float expected[4])
{
    bool ok = true;
    for (int i = 0; i < 4; ++i)
    {
        if (!floatEquals(values[i], expected[i]))
        {
            Serial.print("FAIL ");
            Serial.print(label);
            Serial.print("[");
            Serial.print(i);
            Serial.print("] esperado=");
            Serial.print(expected[i], 2);
            Serial.print(" obtido=");
            Serial.println(values[i], 2);
            ok = false;
        }
    }
    return ok;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== EEPROM MODULE TEST ===");

    pesoAtual = 123.45f;
    ttotal = 678.90f;
    sta_ssid = "TesteSSID";
    sta_password = "Senha1234";
    ap_ssid = "AP_Teste";
    ap_password = "AP_Senha";
    g_wifiConnected = true;
    g_apMode = false;

    for (int i = 0; i < 4; ++i)
    {
        pesoConhecido[i] = expectedPesoConhecido[i];
        fatorEscalaConhecido[i] = expectedFatorEscalaConhecido[i];
    }

    for (int row = 0; row < 20; ++row)
    {
        for (int col = 0; col < 11; ++col)
        {
            tabela[row][col] = "R" + String(row) + "C" + String(col);
        }
    }

    Serial.println("Saving EEPROM data...");
    salvarComEEPROM();
    Serial.println("Save complete.");

    // Clear values to verify reload.
    pesoAtual = 0.0f;
    ttotal = 0.0f;
    sta_ssid = "";
    sta_password = "";
    ap_ssid = "";
    ap_password = "";
    g_wifiConnected = false;
    g_apMode = false;
    for (int i = 0; i < 4; ++i)
    {
        pesoConhecido[i] = 0.0f;
        fatorEscalaConhecido[i] = 0.0f;
    }
    for (int row = 0; row < 20; ++row)
    {
        for (int col = 0; col < 11; ++col)
        {
            tabela[row][col] = "";
        }
    }

    Serial.println("Loading EEPROM data...");
    carregarComEEPROM();
    Serial.println("Load complete.");

    Serial.print("pesoAtual = ");
    Serial.println(pesoAtual, 2);
    Serial.print("ttotal = ");
    Serial.println(ttotal, 2);
    Serial.print("sta_ssid = ");
    Serial.println(sta_ssid);
    Serial.print("sta_password = ");
    Serial.println(sta_password);
    Serial.print("ap_ssid = ");
    Serial.println(ap_ssid);
    Serial.print("ap_password = ");
    Serial.println(ap_password);
    Serial.print("g_wifiConnected = ");
    Serial.println(g_wifiConnected ? "true" : "false");
    Serial.print("g_apMode = ");
    Serial.println(g_apMode ? "true" : "false");

    printFloatArray("pesoConhecido", pesoConhecido);
    printFloatArray("fatorEscalaConhecido", fatorEscalaConhecido);

    bool pesoConhecidoOk = validateFloatArray("pesoConhecido", pesoConhecido, expectedPesoConhecido);
    bool fatorEscalaConhecidoOk = validateFloatArray("fatorEscalaConhecido", fatorEscalaConhecido, expectedFatorEscalaConhecido);

    Serial.print("Teste pesoConhecido: ");
    Serial.println(pesoConhecidoOk ? "PASS" : "FAIL");
    Serial.print("Teste fatorEscalaConhecido: ");
    Serial.println(fatorEscalaConhecidoOk ? "PASS" : "FAIL");

    Serial.println("Tabela sample:");
    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 11; ++col)
        {
            Serial.print(tabela[row][col]);
            Serial.print(" ");
        }
        Serial.println();
    }
}

void loop()
{
    delay(1000);
}

