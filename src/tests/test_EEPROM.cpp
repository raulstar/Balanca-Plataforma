#include <Arduino.h>
#include "EEPROM_Module.hpp"

int contadorRegistro = 0;
float pesoAtual = 0.0f;
float ttotal = 0.0f;
int contEixo = 1;
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

static String expectedTabelaValue(int row, int col)
{
    return "R" + String(row) + "C" + String(col);
}

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

static bool validateTabela()
{
    bool ok = true;
    for (int row = 0; row < 20; ++row)
    {
        for (int col = 0; col < 11; ++col)
        {
            String expected = expectedTabelaValue(row, col);
            if (tabela[row][col] != expected)
            {
                Serial.print("FAIL tabela[");
                Serial.print(row);
                Serial.print("][");
                Serial.print(col);
                Serial.print("] esperado=");
                Serial.print(expected);
                Serial.print(" obtido=");
                Serial.println(tabela[row][col]);
                ok = false;
            }
        }
    }
    return ok;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== EEPROM MODULE TEST ===");

    contadorRegistro = 7;
    pesoAtual = 123.45f;
    ttotal = 678.90f;
    contEixo = 3;
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
            tabela[row][col] = expectedTabelaValue(row, col);
        }
    }

    Serial.println("Saving EEPROM data...");
    salvarComEEPROM();
    Serial.println("Save complete.");

    // Clear values to verify reload.
    contadorRegistro = 0;
    pesoAtual = 0.0f;
    ttotal = 0.0f;
    contEixo = 0;
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

    Serial.print("contadorRegistro = ");
    Serial.println(contadorRegistro);
    Serial.print("pesoAtual = ");
    Serial.println(pesoAtual, 2);
    Serial.print("ttotal = ");
    Serial.println(ttotal, 2);
    Serial.print("contEixo = ");
    Serial.println(contEixo);
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

    bool contadorRegistroOk = contadorRegistro == 7;
    bool contEixoOk = contEixo == 3;
    bool pesoConhecidoOk = validateFloatArray("pesoConhecido", pesoConhecido, expectedPesoConhecido);
    bool fatorEscalaConhecidoOk = validateFloatArray("fatorEscalaConhecido", fatorEscalaConhecido, expectedFatorEscalaConhecido);
    bool tabelaOk = validateTabela();

    Serial.print("Teste contadorRegistro: ");
    Serial.println(contadorRegistroOk ? "PASS" : "FAIL");
    Serial.print("Teste contEixo: ");
    Serial.println(contEixoOk ? "PASS" : "FAIL");
    Serial.print("Teste pesoConhecido: ");
    Serial.println(pesoConhecidoOk ? "PASS" : "FAIL");
    Serial.print("Teste fatorEscalaConhecido: ");
    Serial.println(fatorEscalaConhecidoOk ? "PASS" : "FAIL");
    Serial.print("Teste tabela: ");
    Serial.println(tabelaOk ? "PASS" : "FAIL");

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
    // Check for serial command to clear EEPROM
    if (Serial.available() > 0)
    {
        String command = Serial.readStringUntil('\n');
        command.trim(); // Remove any whitespace including newline/carriage return
        if (command == "c")
        {
            Serial.println("Received CLEAR_EEPROM command. Clearing EEPROM...");
            limparEEPROM();
            Serial.println("EEPROM clear command completed.");
        }
        else
        {
            // Echo unknown commands for feedback
            Serial.print("Unknown command: ");
            Serial.println(command);
        }
    }
    delay(100); // Short delay to allow serial processing
}

