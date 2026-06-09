#include "EEPROM_Module.hpp"
#include <EEPROM.h>
#include <Preferences.h>
#include <math.h>
#include "../HX711_Module/HX711_Module.hpp"

float offset = 0.0f;
float sensorKnownWeight = 0.0f;
int sensorId = 0;
float defaultKnownWeight[4] = {84000.0f, 84000.0f, 84000.0f, 84000.0f};
float defaultScaleFactor[4] = {-5412.36425781f, -5410.0f, -5420.0f, -5400.0f};

static const int EEPROM_SIZE = 8192;
static const int MAX_STR_LEN = 32;
static const int TABLE_ROWS = 20;
static const int TABLE_COLS = 11;

static bool fatorEscalaValido(float value)
{
    return !isnan(value) && !isinf(value) && fabs(value) > 0.000001f && fabs(value) < 10000000.0f;
}

static void salvarFatoresPreferences()
{
    Preferences preferences;
    if (!preferences.begin("calibracao", false))
    {
        Serial.println("ERRO: falha ao abrir Preferences para salvar calibracao.");
        return;
    }

    for (int i = 0; i < 4; ++i)
    {
        char key[12];
        snprintf(key, sizeof(key), "fator%d", i);
        preferences.putFloat(key, novoFator[i]);
    }

    preferences.putFloat("scale", scale_factor);
    preferences.end();
    Serial.println("Fatores de escala salvos tambem em Preferences.");
}

static bool carregarFatoresPreferences()
{
    Preferences preferences;
    if (!preferences.begin("calibracao", true))
    {
        Serial.println("ERRO: falha ao abrir Preferences para carregar calibracao.");
        return false;
    }

    bool carregouAlgumFator = false;

    for (int i = 0; i < 4; ++i)
    {
        char key[12];
        snprintf(key, sizeof(key), "fator%d", i);

        float fator = preferences.getFloat(key, NAN);
        if (fatorEscalaValido(fator))
        {
            novoFator[i] = fator;
            carregouAlgumFator = true;
        }
    }

    float fatorGlobal = preferences.getFloat("scale", NAN);
    if (fatorEscalaValido(fatorGlobal))
    {
        scale_factor = fatorGlobal;
    }
    else if (fatorEscalaValido(novoFator[0]))
    {
        scale_factor = novoFator[0];
    }

    preferences.end();
    return carregouAlgumFator;
}

static void salvarString(int &addr, const String &value, int maxLen)
{
    char buffer[maxLen];
    memset(buffer, 0, maxLen);
    value.toCharArray(buffer, maxLen);
    for (int i = 0; i < maxLen; ++i)
    {
        EEPROM.write(addr++, buffer[i]);
    }
}

static String carregarString(int &addr, int maxLen)
{
    char buffer[MAX_STR_LEN + 1];
    for (int i = 0; i < maxLen; ++i)
    {
        buffer[i] = EEPROM.read(addr++);
    }
    buffer[maxLen] = '\0';
    return String(buffer);
}

void salvarComEEPROM()
{
    if (!EEPROM.begin(EEPROM_SIZE))
    {
        Serial.println("ERRO: falha ao iniciar EEPROM para salvar.");
        return;
    }

    int addr = 0;
    EEPROM.put(addr, pesoAtual);
    Serial.print("Salvando pesoAtual no endereço: ");
    Serial.print(addr);
    Serial.print(" Valor: ");
    Serial.println(pesoAtual);
    addr += sizeof(pesoAtual);

    EEPROM.put(addr, ttotal);
    Serial.print("Salvando ttotal no endereço: ");
    Serial.print(addr);
    Serial.print(" Valor: ");
    Serial.println(ttotal);
    addr += sizeof(ttotal);

    EEPROM.put(addr, g_wifiConnected);
    Serial.print("Salvando g_wifiConnected no endereço: ");
    Serial.print(addr);
    Serial.print(" Valor: ");
    Serial.println(g_wifiConnected);
    addr += sizeof(g_wifiConnected);

    EEPROM.put(addr, g_apMode);
    Serial.print("Salvando g_apMode no endereço: ");
    Serial.print(addr);
    Serial.print(" Valor: ");
    Serial.println(g_apMode);
    addr += sizeof(g_apMode);

    EEPROM.put(addr, offset);
    Serial.print("Salvando offset no endereço: ");
    Serial.print(addr);
    Serial.print(" Valor: ");
    Serial.println(offset);
    addr += sizeof(offset);

    EEPROM.put(addr, scale_factor);
    Serial.print("Salvando scale_factor no endereço: ");
    Serial.print(addr);
    Serial.print(" Valor: ");
    Serial.println(scale_factor);
    addr += sizeof(scale_factor);

    EEPROM.put(addr, sensorKnownWeight);
    Serial.print("Salvando sensorKnownWeight no endereço: ");
    Serial.print(addr);
    Serial.print(" Valor: ");
    Serial.println(sensorKnownWeight);
    addr += sizeof(sensorKnownWeight);

    EEPROM.put(addr, sensorId);
    Serial.print("Salvando sensorId no endereço: ");
    Serial.print(addr);
    Serial.print(" Valor: ");
    Serial.println(sensorId);
    addr += sizeof(sensorId);

    for (int i = 0; i < 4; ++i)
    {
        EEPROM.put(addr, defaultKnownWeight[i]);
        addr += sizeof(defaultKnownWeight[i]);
    }

    for (int i = 0; i < 4; ++i)
    {
        float fatorSalvar = novoFator[i];

        if (!fatorEscalaValido(fatorSalvar))
        {
            float fatorJaSalvo = 0.0f;
            EEPROM.get(addr, fatorJaSalvo);

            if (fatorEscalaValido(fatorJaSalvo))
            {
                fatorSalvar = fatorJaSalvo;
            }
            else
            {
                fatorSalvar = defaultScaleFactor[i];
            }

            novoFator[i] = fatorSalvar;
        }

        EEPROM.put(addr, fatorSalvar);
        Serial.print("Salvando fator de escala [");
        Serial.print(i);
        Serial.print("] no endereço: ");
        Serial.print(addr);
        Serial.print(" Valor: ");
        Serial.println(fatorSalvar, 8);
        addr += sizeof(novoFator[i]);
    }

    salvarString(addr, sta_ssid, MAX_STR_LEN);
    salvarString(addr, sta_password, MAX_STR_LEN);
    salvarString(addr, ap_ssid, MAX_STR_LEN);
    salvarString(addr, ap_password, MAX_STR_LEN);
    salvarString(addr, tplaca, MAX_STR_LEN);

    for (int row = 0; row < TABLE_ROWS; ++row)
    {
        for (int col = 0; col < TABLE_COLS; ++col)
        {
            salvarString(addr, tabela[row][col], MAX_STR_LEN);
        }
    }
    bool commitOk = EEPROM.commit();
    Serial.println(commitOk ? "EEPROM salva e commit OK." : "ERRO: EEPROM.commit() falhou.");
    EEPROM.end();

    salvarFatoresPreferences();
}

void carregarComEEPROM()
{
    if (!EEPROM.begin(EEPROM_SIZE))
    {
        Serial.println("ERRO: falha ao iniciar EEPROM para carregar.");
        return;
    }

    int addr = 0;
    EEPROM.get(addr, pesoAtual);
    addr += sizeof(pesoAtual);

    EEPROM.get(addr, ttotal);
    addr += sizeof(ttotal);

    EEPROM.get(addr, g_wifiConnected);
    addr += sizeof(g_wifiConnected);

    EEPROM.get(addr, g_apMode);
    addr += sizeof(g_apMode);

    EEPROM.get(addr, offset);
    addr += sizeof(offset);

    EEPROM.get(addr, scale_factor);
    if (!fatorEscalaValido(scale_factor))
    {
        scale_factor = defaultScaleFactor[0];
    }
    addr += sizeof(scale_factor);
    Serial.print("Scale Factor carregado: ");
    Serial.println(scale_factor, 8);

    EEPROM.get(addr, sensorKnownWeight);
    addr += sizeof(sensorKnownWeight);

    EEPROM.get(addr, sensorId);
    addr += sizeof(sensorId);

    for (int i = 0; i < 4; ++i)
    {
        EEPROM.get(addr, defaultKnownWeight[i]);
        addr += sizeof(defaultKnownWeight[i]);
    }

    for (int i = 0; i < 4; ++i)
    {
        EEPROM.get(addr, novoFator[i]);
        if (!fatorEscalaValido(novoFator[i]))
        {
            novoFator[i] = defaultScaleFactor[i];
        }
        Serial.print("Carregando fator de escala [");
        Serial.print(i);
        Serial.print("] do endereço: ");
        Serial.print(addr);
        Serial.print(" Valor: ");
        Serial.println(novoFator[i], 8);
        addr += sizeof(novoFator[i]);
    }

    bool fatoresPreferencesOk = carregarFatoresPreferences();
    if (fatoresPreferencesOk)
    {
        Serial.println("Fatores de escala carregados de Preferences.");
        for (int i = 0; i < 4; ++i)
        {
            Serial.print("Preferences fator de escala [");
            Serial.print(i);
            Serial.print("] Valor: ");
            Serial.println(novoFator[i], 8);
        }
    }

    if (!fatorEscalaValido(scale_factor) && fatorEscalaValido(novoFator[0]))
    {
        scale_factor = novoFator[0];
    }

    sta_ssid = carregarString(addr, MAX_STR_LEN);
    sta_password = carregarString(addr, MAX_STR_LEN);
    ap_ssid = carregarString(addr, MAX_STR_LEN);
    ap_password = carregarString(addr, MAX_STR_LEN);
    tplaca = carregarString(addr, MAX_STR_LEN);

    for (int row = 0; row < TABLE_ROWS; ++row)
    {
        for (int col = 0; col < TABLE_COLS; ++col)
        {
            tabela[row][col] = carregarString(addr, MAX_STR_LEN);
        }
    }

    EEPROM.end();
}
