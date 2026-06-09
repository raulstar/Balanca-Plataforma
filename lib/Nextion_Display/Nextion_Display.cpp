#include "Nextion_Display.hpp"
#include "../WiFi_Server/WiFi_Server.hpp"
#include "../EEPROM_Module/EEPROM_Module.hpp"

// ======================================================
// SERIAL NEXTION
// ======================================================
HardwareSerial NEXTION_SERIAL(1);
SensorConfig *_sensores = nullptr;
int _numSensores = 0;
volatile bool imprimir = false;
SemaphoreHandle_t xSensorMutex = nullptr;

void setSensores(SensorConfig *sens, int num)
{
    _sensores = sens;
    _numSensores = num;
}

// ======================================================
// VARIÁVEIS GLOBAIS
// ======================================================
String placaVeiculo = "";
String dataRegistro = "";
bool calibrando1 = false;
bool zero = false;
bool salvarRegistro = false;

String tdata = "";
String thora = "";
String tbateria = "90%";
String bplatafor1;
String bplatafor2;
String bplatafor3;
String bplatafor4;
String tpeso1 = "";
String tpeso2 = "";
String tpeso3 = "";
String tpeso4 = "";
String calib = "";
int indexCalib;

String tplaca = "";

float pesoAtual = 0.0;
float ttotal = 0.0;
float ttara = 0.0;
int eixo1 = 0;
int eixo2 = 0;
int eixo3 = 0;
int eixo4 = 0;
int eixo5 = 0;

String tabela[20][11];

int linhaAtual = 0;
int contadorRegistro = 0;
int contEixo = 1;

String last_thora = "";
String last_tdata = "";
String last_tplaca = "";
String last_tbateria = "";
String last_tpeso1 = "";
String last_tpeso2 = "";
String last_tpeso3 = "";
String last_tpeso4 = "";

float last_pesoAtual = -999999.0;
float last_ttara = -999999.0;
float last_ttotal = -999999.0;

int last_contadorRegistro = 0;
int last_contEixo = 0;

// WiFi status variables tracking
String last_sta_ssid = "";
String last_sta_password = "";
String last_ap_ssid = "";
String last_ap_password = "";
bool last_g_wifiConnected = false;
bool last_g_apMode = false;
bool wifi_initialized = false;

int eixo = 0;
int peixo = 0;

int peixo1;
int peixo2;
int peixo3;
int peixo4;
int peixo5;
int peixo6;

// ======================================================
// ENVIA TEXTO PARA NEXTION
// ======================================================
void setNextionText(String objName, String value)
{
    NEXTION_SERIAL.print(objName);
    NEXTION_SERIAL.print(".txt=\"");
    NEXTION_SERIAL.print(value);
    NEXTION_SERIAL.print("\"");

    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
}

// ======================================================
// INICIALIZA NEXTION
// ======================================================
void initNextion()
{
    NEXTION_SERIAL.begin(115200, SERIAL_8N1, NEXTION_RX, NEXTION_TX);

    Serial.println("Nextion iniciado");

    // Send initial WiFi configuration values to display
    setNextionText("page3.gssid", sta_ssid);
    last_sta_ssid = sta_ssid;

    setNextionText("page3.gpassword", sta_password);
    last_sta_password = sta_password;

    setNextionText("page3.gapssi", ap_ssid);
    last_ap_ssid = ap_ssid;

    setNextionText("page3.gappssword", ap_password);
    last_ap_password = ap_password;

    wifi_initialized = true;

    updateDisplay();
}

// ======================================================
// ATUALIZA DISPLAY
// ======================================================
void updateDisplay()
{
    // Hora
    if (thora != last_thora)
    {
        setNextionText("thora", thora);
        last_thora = thora;
    }

    // Data
    if (tdata != last_tdata)
    {
        setNextionText("tdata", tdata);
        last_tdata = tdata;
    }

    // Placa
    if (tplaca != last_tplaca)
    {
        setNextionText("tplaca", tplaca);
        last_tplaca = tplaca;
        Serial.print("Placa no display: ");
        Serial.println(tplaca);
    }

    // Sensores
    if (_sensores != nullptr)
    {
        for (int i = 0; i < _numSensores; i++)
        {
            String sensorVal = String(_sensores[i].sensor->getKg(), 1);
            String objName = "tPeso" + String(i + 1);

            // Sempre envia o texto atualizado para o display
            setNextionText(objName, sensorVal);

            switch (i)
            {
            case 0:
                tpeso1 = sensorVal;
                break;
            case 1:
                tpeso2 = sensorVal;
                break;
            case 2:
                tpeso3 = sensorVal;
                break;
            case 3:
                tpeso4 = sensorVal;
                break;
            }
        }
        // Serial.println(tpeso1 + " | " + tpeso2 + " | " + tpeso3 + " | " + tpeso4);
    }

    // Peso Atual
    // Atualiza independentemente se houve mudança ou não, pois a tara pode ter zerado
    // mas o valor absoluto (0.0) ainda pode ser considerado igual ao anterior.
    setNextionText("tPeso", String(pesoAtual, 1) + " kg");
    last_pesoAtual = pesoAtual;
    // Serial.println(pesoAtual, 1);

    // Total
    if (ttotal != last_ttotal)
    {
        setNextionText("ttotal", String(ttotal, 1) + " kg");
        last_ttotal = ttotal;
    }

    // Tara
    if (ttara != last_ttara)
    {
        setNextionText("ttara", String(ttara, 1) + " kg");
        last_ttara = ttara;
    }

    // Contador
    if (contadorRegistro != last_contadorRegistro)
    {
        setNextionText("tn0", String(contadorRegistro));
        last_contadorRegistro = contadorRegistro;
    }

    // Contador de eixos
    if (contEixo != last_contEixo)
    {
        setNextionText("tcontEixo", String(contEixo));
        last_contEixo = contEixo;
    }

    // Sensor display fallback values
    if (tpeso1 != last_tpeso1)
    {
        setNextionText("tpeso1", tpeso1 + " kg");
        setNextionText("page4.tpeso1", tpeso1 + " kg");
        last_tpeso1 = tpeso1;
    }
    if (tpeso2 != last_tpeso2)
    {
        setNextionText("tpeso2", tpeso2 + " kg");
        setNextionText("page4.tpeso2", tpeso2 + " kg");
        last_tpeso2 = tpeso2;
    }
    if (tpeso3 != last_tpeso3)
    {
        setNextionText("tpeso3", tpeso3 + " kg");
        setNextionText("page4.tpeso3", tpeso3 + " kg");
        last_tpeso3 = tpeso3;
    }
    if (tpeso4 != last_tpeso4)
    {
        setNextionText("tpeso4", tpeso4 + " kg");
        setNextionText("page4.tpeso4", tpeso4 + " kg");
        last_tpeso4 = tpeso4;
    }

    // WiFi Status - STA SSID
    if (sta_ssid != last_sta_ssid)
    {
        setNextionText("page6.gssid", sta_ssid);
        last_sta_ssid = sta_ssid;
    }

    // WiFi Status - STA Password
    if (sta_password != last_sta_password)
    {
        setNextionText("page6.gpassword", sta_password);
        last_sta_password = sta_password;
    }

    // WiFi Status - AP SSID
    if (ap_ssid != last_ap_ssid)
    {
        setNextionText("page6.gapssi", ap_ssid);
        last_ap_ssid = ap_ssid;
    }

    // WiFi Status - AP Password
    if (ap_password != last_ap_password)
    {
        setNextionText("page6.gpasspword", ap_password);
        last_ap_password = ap_password;
    }

    // WiFi Status - Connection Status
    if (g_wifiConnected != last_g_wifiConnected)
    {
        last_g_wifiConnected = g_wifiConnected;
        Serial.print("WiFi Connected status changed to: ");
        Serial.println(g_wifiConnected ? "true" : "false");
    }

    // WiFi Status - AP Mode
    if (g_apMode != last_g_apMode)
    {
        last_g_apMode = g_apMode;
        Serial.print("AP Mode status changed to: ");
        Serial.println(g_apMode ? "true" : "false");
    }
}

// ======================================================
// PROCESSA COMANDOS
// ======================================================
void processNextionCommands()
{
    // Non-blocking read: accumulate bytes and process complete lines terminated by '\n'.
    static String commandBuf = "";

    while (NEXTION_SERIAL.available())
    {
        char c = (char)NEXTION_SERIAL.read();

        if (c == '\n')
        {
            String command = commandBuf;
            command.trim();

            if (command.length() > 0)
            {

                if (command.indexOf("bsom") >= 0)
                {
                    handle_bsom();
                    Serial.println("Evento [bsom]");
                }
                else if (command.startsWith("gcalib:"))
                {
                    Serial.println("Evento [gcalib]");
                    salvarRegistro = true;
                    int colonPos = command.indexOf(':');
                    String valorStr = command.substring(colonPos + 1);
                    valorStr.trim();
                    pesoCalibracao1 = atof(valorStr.c_str());

                    Serial.print("valorStr: ");
                    Serial.println(valorStr);
                    Serial.print("peso Calibracao1: ");
                    Serial.println(pesoCalibracao1);
                }
                else if (command.indexOf("bzero") >= 0)
                {
                    handle_bzero();
                    Serial.println("Evento [bzero]");
                }
                else if (command.indexOf("bsalva") >= 0)
                {
                    handle_bsalvar();
                    salvarRegistro = true;
                    Serial.println("Evento [bsalva]");
                }
                else if (command.indexOf("blimpar") >= 0)
                {
                    handle_blimpar();
                    Serial.println("Evento [blimpar]");
                }
                else if (command.indexOf("bcalib") >= 0)
                {
                    handle_bcalib(command);
                    Serial.println("Evento [bcalib]");
                }
                else if (command.indexOf("imprimi") >= 0)
                {
                    imprimir = true;
                    Serial.println("Evento [imprimir]");
                }
                else if (command.indexOf("bplatafor1") >= 0)
                {
                    indexCalib = 1;
                    Serial.println("Evento [bplatafor1]");
                    if (xSensorMutex && xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        _sensores[0].sensor->calibra(pesoCalibracao1);
                        xSemaphoreGive(xSensorMutex);
                        Serial.println("Sensor 1 calibrado");
                    }
                }
                else if (command.indexOf("bplatafor2") >= 0)
                {
                    indexCalib = 2;
                    Serial.println("Evento [bplatafor2]");
                    if (xSensorMutex && xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        _sensores[1].sensor->calibra(pesoCalibracao1);
                        xSemaphoreGive(xSensorMutex);
                        Serial.println("Sensor 2 calibrado");
                    }
                }
                else if (command.indexOf("bplatafor3") >= 0)
                {
                    indexCalib = 3;
                    Serial.println("Evento [bplatafor3]");
                    if (xSensorMutex && xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        _sensores[2].sensor->calibra(pesoCalibracao1);
                        xSemaphoreGive(xSensorMutex);
                        Serial.println("Sensor 3 calibrado");
                    }
                }
                else if (command.indexOf("bplatafor4") >= 0)
                {
                    indexCalib = 4;
                    Serial.println("Evento [bplatafor4]");
                    if (xSensorMutex && xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        _sensores[3].sensor->calibra(pesoCalibracao1);
                        xSemaphoreGive(xSensorMutex);
                        Serial.println("Sensor 4 calibrado");
                    }
                }
                else if (command.indexOf("calib") >= 0)
                {
                    Serial.println("Evento [calib]");
                    salvarRegistro = true;
                    int pos = command.indexOf(':');

                    if (pos > 0)
                    {
                        String valorStr = command.substring(pos + 1);

                        pesoCalibracao1 = valorStr.toFloat() * 1000.0f; // Converter kg para gramas

                        Serial.print("Peso de calibração " + String(indexCalib) + ": ");
                        Serial.println(pesoCalibracao1 / 1000.0f, 2);
                        Serial.print("calibrando");
                        Serial.print(indexCalib);
                        Serial.print(" ");
                        Serial.println(pesoCalibracao1 / 1000.0f, 2);
                        calibrando1 = true;
                    }
                }
                else if (command.startsWith("placa"))
                {
                    Serial.println("Evento [placa]");
                    int pos = command.indexOf(':');
                    String valorStr;

                    if (pos >= 0)
                    {
                        valorStr = command.substring(pos + 1);
                    }
                    else
                    {
                        valorStr = command.substring(5);
                    }

                    valorStr.trim();
                    placaVeiculo = valorStr;
                    tplaca = valorStr;
                    Serial.print("Placa recebida: ");
                    Serial.println(placaVeiculo);
                }
                else if (command.startsWith("thora:"))
                {
                    Serial.println("Evento [thora]");

                    String valorStr = command.substring(6);
                    valorStr.trim();
                    thora = valorStr;

                    Serial.print("hora recebida: ");
                    Serial.println(thora);
                }
                else if (command.startsWith("tdata:"))
                {
                    Serial.println("Evento [tdata]");

                    String valorStr = command.substring(6);
                    valorStr.trim();
                    tdata = valorStr;

                    Serial.print("data recebida: ");
                    Serial.println(tdata);
                }
                else
                {
                    handle_bgeneric(command);
                }
            }

            // Clear buffer for next line
            commandBuf = "";
        }
        else if (c != '\r')
        {
            // Append char, but guard buffer size to avoid runaway memory use
            commandBuf += c;
            if (commandBuf.length() > 512)
            {
                commandBuf = commandBuf.substring(commandBuf.length() - 512);
            }
        }
    }
}

// ======================================================
// EVENTO BZERO
// ======================================================
void handle_bzero()
{
    Serial.println("Evento [bzero]");
    zero = true;
    pesoAtual = 0.0;

    setNextionText("tPeso", String(pesoAtual, 1));
}

// ======================================================
// EVENTO BSOM
// ======================================================
void handle_bsom()
{
    Serial.println("Evento [bsom]");

    ttotal += pesoAtual;
    peixo = (int)pesoAtual;

    switch (contEixo)
    {
    case 0:
        eixo1 = peixo;
        peixo1 = peixo;
        break;
    case 1:
        eixo2 = peixo;
        peixo2 = peixo;
        break;
    case 2:
        eixo3 = peixo;
        peixo3 = peixo;
        break;
    case 3:
        eixo4 = peixo;
        peixo4 = peixo;
        break;
    case 4:
        eixo5 = peixo;
        peixo5 = peixo;
        break;
    default:
        peixo6 = peixo;
        break;
    }

    contEixo++;
    eixo++;

    setNextionText("ttotal", String(ttotal, 1));
}

// ======================================================
// EVENTO BSALVAR
// ======================================================
void handle_bsalvar()
{
    Serial.println("Evento [bsalvar]");

    String objTN[7] =
        {
            "page1.tn0", "page1.tn1", "page1.tn2", "page1.tn3", "page1.tn4", "page1.tn5", "page1.tn6"};

    String objPLACA[7] =
        {
            "page1.tplaca0", "page1.tplaca1", "page1.tplaca2", "page1.tplaca3", "page1.tplaca4", "page1.tplaca5", "page1.tplaca6"};

    String objDATA[7] =
        {
            "page1.tdata0", "page1.tdata1", "page1.tdata2", "page1.tdata3", "page1.tdata4", "page1.tdata5", "page1.tdata6"};

    String objHORA[7] =
        {
            "page1.thora0", "page1.thora1", "page1.thora2", "page1.thora3", "page1.thora4", "page1.thora5", "page1.thora6"};
    String objEIXO[7] =
        {
            "page1.contEixo0", "page1.contEixo1", "page1.contEixo2", "page1.contEixo3", "page1.contEixo4", "page1.contEixo5", "page1.contEixo6"};

    String objTOTAL[7] =
        {
            "page1.ttotal0", "page1.ttotal1", "page1.ttotal2", "page1.ttotal3", "page1.ttotal4", "page1.ttotal5", "page1.ttotal6"};

    String objTARA[7] =
        {
            "page1.ttara0", "page1.ttara1", "page1.ttara2", "page1.ttara3", "page1.ttara4", "page1.ttara5", "page1.ttara6"};

    // for (int i = 6; i > 0; i--)
    // {
    //     for (int j = 0; j < 11; j++)
    //     {
    //         tabela[i][j] = tabela[i - 1][j];
    //     }
    // }

    tabela[contadorRegistro][0] = String(contadorRegistro);
    tabela[contadorRegistro][1] = tplaca;
    tabela[contadorRegistro][2] = tdata;
    tabela[contadorRegistro][3] = thora;
    tabela[contadorRegistro][4] = contEixo;
    tabela[contadorRegistro][5] = String((float)eixo1, 1);
    tabela[contadorRegistro][6] = String((float)eixo2, 1);
    tabela[contadorRegistro][7] = String((float)eixo3, 1);
    tabela[contadorRegistro][8] = String((float)eixo4, 1);
    tabela[contadorRegistro][9] = String((float)eixo5, 1);
    tabela[contadorRegistro][10] = String(ttotal, 1);
    Serial.print("//////////////////////////////////////");
    Serial.println("contadorRegistro");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][0]);
    Serial.print("tplaca");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][1]);
    Serial.print("tdata");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][2]);
    Serial.print("thora");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][3]);
    Serial.print("contaeixo");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][4]);
    Serial.print("eixo1");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][5]);
    Serial.print("eixo2");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][6]);
    Serial.print("eixo3");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][7]);
    Serial.print("eixo4");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][8]);
    Serial.print("eixo5");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][9]);
    Serial.print("ttotal");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][10]);
    Serial.print("ttara");
    Serial.print(": ");
    Serial.println(tabela[contadorRegistro][11]);

    setNextionText(objTN[contadorRegistro], tabela[contadorRegistro][0]);

    setNextionText(objPLACA[contadorRegistro], tabela[contadorRegistro][1]);

    setNextionText(objDATA[contadorRegistro], tabela[contadorRegistro][2]);

    setNextionText(objHORA[contadorRegistro], tabela[contadorRegistro][3]);

    setNextionText(objEIXO[contadorRegistro], tabela[contadorRegistro][4]);

    setNextionText(objTOTAL[contadorRegistro], tabela[contadorRegistro][9]);

    setNextionText(objTARA[contadorRegistro], tabela[contadorRegistro][10]);

    contadorRegistro++;
    salvarComEEPROM();
    delay(2);
}

// ======================================================
// EVENTO BLIMPAR
// ======================================================
void handle_blimpar()
{
    Serial.println("Evento [blimpar]");

    pesoAtual = 0.0;
    ttotal = 0.0;
    ttara = 0.0;
    tplaca = "";
    placaVeiculo = "";
    contEixo = 1;
    eixo = 0;
    peixo = 0;
    eixo1 = 0;
    eixo2 = 0;
    eixo3 = 0;
    eixo4 = 0;
    eixo5 = 0;
    peixo1 = 0;
    peixo2 = 0;
    peixo3 = 0;
    peixo4 = 0;
    peixo5 = 0;
    peixo6 = 0;

    updateDisplay();
}

// ======================================================
// EVENTOS GENÉRICOS
// ======================================================
void handle_bgeneric(String cmd)
{
    Serial.print("Outro comando: ");
    Serial.println(cmd);
}

// ======================================================
// EVENTO BCALIB
// ======================================================
void handle_bcalib(String cmd)
{
    Serial.println("Evento [bcalib]");

    // Exemplo recebido:
    // bcalib:41A00000

    int pos = cmd.indexOf(':');

    if (pos > 0)
    {
        String hexValue = cmd.substring(pos + 1);

        uint32_t hexInt = strtoul(hexValue.c_str(), NULL, 16);

        float valor;

        memcpy(&valor, &hexInt, sizeof(valor));

        pesoCalibracao1 = valor * 1000.0f; // Converter kg para gramas

        Serial.print("pesoCalibracao1 = ");
        Serial.println(pesoCalibracao1 / 1000.0f, 4);
        salvarComEEPROM();
    }
}
