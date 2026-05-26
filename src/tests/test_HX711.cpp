#include <Arduino.h>
#include "HX711_Module.hpp"

/////////////////////////////////////////////////////////////////////////////
// PINOS
/////////////////////////////////////////////////////////////////////////////

#define M0 19
#define M1 21

#define m1 digitalWrite(M1, LOW)
#define m0 digitalWrite(M0, LOW)

/////////////////////////////////////////////////////////////////////////////
// UART SENSOR
/////////////////////////////////////////////////////////////////////////////

HardwareSerial SerialPort(2);

/////////////////////////////////////////////////////////////////////////////
// SENSOR HX711
/////////////////////////////////////////////////////////////////////////////

SensorBalanca sensor1(SerialPort, "S1");
SensorBalanca sensor2(SerialPort, "S2");
SensorBalanca sensor3(SerialPort, "S3");
SensorBalanca sensor4(SerialPort, "S4");

struct SensorConfig {
    SensorBalanca &sensor;
    String prefixo;
};

SensorConfig sensores[] = {
    {sensor1, "S1"},
    {sensor2, "S2"},
    {sensor3, "S3"},
    {sensor4, "S4"}
};
const int numSensores = sizeof(sensores) / sizeof(sensores[0]);

/////////////////////////////////////////////////////////////////////////////
// PESO CALIBRAÇÃO
/////////////////////////////////////////////////////////////////////////////

float pesoCalibracao1 = 78000.0f;

/////////////////////////////////////////////////////////////////////////////
// SETUP
/////////////////////////////////////////////////////////////////////////////

void setup()
{
    pinMode(M1, OUTPUT);
    pinMode(M0, OUTPUT);

    m1;
    m0;

    Serial.begin(115200);

    SerialPort.begin(9600, SERIAL_8N1, 16, 17);

    delay(1000);

    Serial.println("\n=== SISTEMA BALANCA ===");

    Serial.println("Comandos:");
    Serial.println("t -> Tara");
    Serial.println("c -> Calibrar usando peso padrão");
    Serial.println("c5000 -> Calibrar com 5000g");

    Serial.println("========================");
}

/////////////////////////////////////////////////////////////////////////////
// LOOP
/////////////////////////////////////////////////////////////////////////////

void loop()
{
    //////////////////////////////////////////////////////////////////////////
    // LEITURA SENSOR
    //////////////////////////////////////////////////////////////////////////

    static String bufferSerial;
    while (SerialPort.available())
    {
        char c = SerialPort.read();
        if (c == '\n')
        {
            for (int i = 0; i < numSensores; i++)
            {
                if (sensores[i].sensor.processaString(bufferSerial))
                {
                    Serial.print(sensores[i].prefixo + " RAW: ");
                    Serial.print(sensores[i].sensor.getRaw(), 3);

                    Serial.print(" | " + sensores[i].prefixo + " KG: ");
                    Serial.print(sensores[i].sensor.getKg(), 3);
                    Serial.println();
                }
            }
            bufferSerial = "";
        }
        else if (c != '\r')
        {
            bufferSerial += c;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // COMANDOS SERIAL
    //////////////////////////////////////////////////////////////////////////

    if (Serial.available())
    {
        String comando = Serial.readStringUntil('\n');

        comando.trim();

        //////////////////////////////////////////////////////////////////////////
        // TARA
        //////////////////////////////////////////////////////////////////////////

        if (comando.equalsIgnoreCase("t"))
        {
            sensor1.tare();
        }

        //////////////////////////////////////////////////////////////////////////
        // CALIBRAÇÃO COM PESO PADRÃO
        //////////////////////////////////////////////////////////////////////////

        else if (comando.equalsIgnoreCase("c"))
        {
            Serial.println("--------------------------------");
            Serial.println("COLOQUE O PESO DE CALIBRACAO...");
            Serial.println("--------------------------------");

            delay(3000);

            sensor1.calibra(pesoCalibracao1);
        }

        //////////////////////////////////////////////////////////////////////////
        // CALIBRAÇÃO COM PESO INFORMADO
        //////////////////////////////////////////////////////////////////////////

        else if (comando.startsWith("c") || comando.startsWith("C"))
        {
            String pesoTexto = comando.substring(1);

            float pesoConhecido = pesoTexto.toFloat();

            if (pesoConhecido <= 0)
            {
                Serial.println("ERRO: peso invalido");

                return;
            }

            Serial.println("--------------------------------");
            Serial.println("AGUARDE ESTABILIZAR...");
            Serial.println("--------------------------------");

            delay(3000);

            sensor1.calibra(pesoConhecido);
        }
    }
}