#include "EEPROM_Module.hpp"
#include <EEPROM.h>
#include "../HX711_Module/HX711_Module.hpp"

float offset = 0.0f;
float sensorKnownWeight = 0.0f;
int sensorId = 0;
float pesoConhecido[4] = {78000.0f, 79000.0f, 80000.0f, 81000.0f};
float fatorEscalaConhecido[4] = {-5801.10f, -5410.0f, -5420.0f, -5400.0f};

static const int EEPROM_SIZE = 8192;
static const int MAX_STR_LEN = 32;
static const int TABLE_ROWS = 20;
static const int TABLE_COLS = 11;

static bool valorFloatValido(float valor)
{
    return !isnan(valor) && !isinf(valor);
}

static int tamanhoEEPROMUsado()
{
    return (sizeof(pesoAtual) + sizeof(ttotal) + sizeof(contEixo) +
            sizeof(g_wifiConnected) + sizeof(g_apMode) +
            sizeof(offset) + sizeof(float) + sizeof(sensorKnownWeight) + sizeof(sensorId) +
            sizeof(pesoConhecido) + sizeof(fatorEscalaConhecido) +
            (5 * MAX_STR_LEN) + (TABLE_ROWS * TABLE_COLS * MAX_STR_LEN));
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
    const int eepromUsada = tamanhoEEPROMUsado();
    if (eepromUsada > EEPROM_SIZE)
    {
        Serial.print("ERRO: tamanho usado pela EEPROM excede EEPROM_SIZE. Usado: ");
        Serial.print(eepromUsada);
        Serial.print(" Limite: ");
        Serial.println(EEPROM_SIZE);
        return;
    }

    if (!EEPROM.begin(EEPROM_SIZE))
    {
        Serial.println("ERRO: EEPROM.begin() falhou. Verifique a particao NVS.");
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

    EEPROM.put(addr, contEixo);
    Serial.print("Salvando contEixo no endereço: ");
    Serial.print(addr);
    Serial.print(" Valor: ");
    Serial.println(contEixo);
    addr += sizeof(contEixo);

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

    // Endereço reservado para compatibilidade do layout antigo.
    // O fator de escala de cada sensor é salvo em fatorEscalaConhecido[0..3]
    // nos endereços 46, 50, 54 e 58.
    addr += sizeof(float);

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
        EEPROM.put(addr, pesoConhecido[i]);
        addr += sizeof(pesoConhecido[i]);
    }

    for (int i = 0; i < 4; ++i)
    {
        EEPROM.put(addr, fatorEscalaConhecido[i]);
        Serial.print("Salvando fator de escala [");
        Serial.print(i);
        Serial.print("] no endereço: ");
        Serial.print(addr);
        Serial.print(" Valor: ");
        Serial.println(fatorEscalaConhecido[i]);
        addr += sizeof(fatorEscalaConhecido[i]);
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
    if (!EEPROM.commit())
    {
        Serial.println("ERRO: EEPROM.commit() falhou. Dados nao foram gravados.");
    }
}

void salvarFatorEEPROM(String scale_factor, int endereco)
{
    int indice = endereco - 1;

    if (indice < 0 || indice >= 4)
    {
        Serial.print("ERRO: endereco/indexCalib invalido para salvar fator EEPROM: ");
        Serial.println(endereco);
        return;
    }

    float fator = scale_factor.toFloat();
    if (!valorFloatValido(fator))
    {
        Serial.print("ERRO: fator de escala invalido para salvar EEPROM: ");
        Serial.println(scale_factor);
        return;
    }

    fatorEscalaConhecido[indice] = fator;
    sensorId = indice;
    ::scale_factor = fator;

    Serial.print("Salvando fator EEPROM do indexCalib ");
    Serial.print(endereco);
    Serial.print(": ");
    Serial.println(fator, 8);

    salvarComEEPROM();
}

void carregarComEEPROM()
{
    const int eepromUsada = tamanhoEEPROMUsado();
    if (eepromUsada > EEPROM_SIZE)
    {
        Serial.print("ERRO: tamanho usado pela EEPROM excede EEPROM_SIZE. Usado: ");
        Serial.print(eepromUsada);
        Serial.print(" Limite: ");
        Serial.println(EEPROM_SIZE);
        return;
    }

    if (!EEPROM.begin(EEPROM_SIZE))
    {
        Serial.println("ERRO: EEPROM.begin() falhou. Verifique a particao NVS.");
        return;
    }

    int addr = 0;
    EEPROM.get(addr, pesoAtual);
    addr += sizeof(pesoAtual);
    if (!valorFloatValido(pesoAtual))
    {
        pesoAtual = 0.0f;
    }

    EEPROM.get(addr, ttotal);
    addr += sizeof(ttotal);
    if (!valorFloatValido(ttotal))
    {
        ttotal = 0.0f;
    }

    EEPROM.get(addr, contEixo);
    addr += sizeof(contEixo);

    EEPROM.get(addr, g_wifiConnected);
    addr += sizeof(g_wifiConnected);

    EEPROM.get(addr, g_apMode);
    addr += sizeof(g_apMode);

    EEPROM.get(addr, offset);
    addr += sizeof(offset);

    // Endereço reservado para compatibilidade do layout antigo.
    // O scale_factor global é derivado de fatorEscalaConhecido[sensorId].
    addr += sizeof(float);

    EEPROM.get(addr, sensorKnownWeight);
    addr += sizeof(sensorKnownWeight);

    EEPROM.get(addr, sensorId);
    addr += sizeof(sensorId);

    for (int i = 0; i < 4; ++i)
    {
        EEPROM.get(addr, pesoConhecido[i]);
        addr += sizeof(pesoConhecido[i]);
    }

    for (int i = 0; i < 4; ++i)
    {
        EEPROM.get(addr, fatorEscalaConhecido[i]);
        Serial.print("Carregando fator de escala [");
        Serial.print(i);
        Serial.print("] do endereço: ");
        Serial.print(addr);
        Serial.print(" Valor: ");
        Serial.println(fatorEscalaConhecido[i]);
        addr += sizeof(fatorEscalaConhecido[i]);
    }

    if (sensorId >= 0 && sensorId < 4)
    {
        scale_factor = fatorEscalaConhecido[sensorId];
    }
    // Serial.print("Scale Factor carregado do fatorEscalaConhecido[");
    // Serial.print(sensorId);
    // Serial.print("]: ");
    // Serial.println(scale_factor);

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
