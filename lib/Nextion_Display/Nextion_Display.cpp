#include "Nextion_Display.hpp"

// UART1
HardwareSerial NEXTION_SERIAL(1);

void initNextion()
{
    NEXTION_SERIAL.begin(9600, SERIAL_8N1, 25, 26);
    delay(500);

    nextionCmd("page 0");
    nextionCmd("tPeso.txt=\"Iniciando...\"");
}

void nextionCmd(const String &cmd)
{
    NEXTION_SERIAL.print(cmd);
    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
}

void lerNextion()
{
    if (!NEXTION_SERIAL.available())
        return;

    String entrada = NEXTION_SERIAL.readStringUntil('\xFF');
    entrada.trim();

    if (entrada == "ZERO")
    {
        handleZero();  // 🔥 chama função do main
    }
}