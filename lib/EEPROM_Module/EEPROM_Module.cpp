#include "EEPROM_Module.hpp"
#include <EEPROM.h>

float offset = 0.0f;
float scale_factor = 0.0f;
float sensorKnownWeight = 0.0f;
int sensorId = 0;
float defaultKnownWeight[4] = {78000.0f, 79000.0f, 80000.0f, 81000.0f};
float defaultScaleFactor[4] = {-5412.36425781f, -5410.0f, -5420.0f, -5400.0f};

static const int EEPROM_SIZE = 4096;
static const int MAX_STR_LEN = 32;
static const int TABLE_ROWS = 20;
static const int TABLE_COLS = 6;

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
    EEPROM.begin(EEPROM_SIZE);

    int addr = 0;
    EEPROM.put(addr, pesoAtual);
    addr += sizeof(pesoAtual);

    EEPROM.put(addr, ttotal);
    addr += sizeof(ttotal);

    EEPROM.put(addr, g_wifiConnected);
    addr += sizeof(g_wifiConnected);

    EEPROM.put(addr, g_apMode);
    addr += sizeof(g_apMode);

    EEPROM.put(addr, offset);
    addr += sizeof(offset);

    EEPROM.put(addr, scale_factor);
    addr += sizeof(scale_factor);

    EEPROM.put(addr, sensorKnownWeight);
    addr += sizeof(sensorKnownWeight);

    EEPROM.put(addr, sensorId);
    addr += sizeof(sensorId);

    for (int i = 0; i < 4; ++i)
    {
        EEPROM.put(addr, defaultKnownWeight[i]);
        addr += sizeof(defaultKnownWeight[i]);
    }

    for (int i = 0; i < 4; ++i)
    {
        EEPROM.put(addr, defaultScaleFactor[i]);
        addr += sizeof(defaultScaleFactor[i]);
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
    Serial.println("EEPROM salva.");
    EEPROM.commit();
}

void carregarComEEPROM()
{
    EEPROM.begin(EEPROM_SIZE);

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
    addr += sizeof(scale_factor);

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
        EEPROM.get(addr, defaultScaleFactor[i]);
        addr += sizeof(defaultScaleFactor[i]);
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
}
