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

SensorBalanca sensor1(SerialPort, "S1", 0);
SensorBalanca sensor2(SerialPort, "S2", 1);
SensorBalanca sensor3(SerialPort, "S3", 2);
SensorBalanca sensor4(SerialPort, "S4", 3);

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

extern const int numSensores;

const int numSensores = sizeof(sensores) / sizeof(sensores[0]);

// Variável global que armazenará a soma dos valores (kg) de todos os sensores.
float pesoAtual = 0.0f;

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

    delay(600);

    Serial.println("\n=== SISTEMA BALANCA ===");

    Serial.println("Comandos:");
    Serial.println("t -> Tara");
    Serial.println("c -> Calibrar usando peso padrão");
    //Serial.println("c5000 -> Calibrar com 5000g");

    Serial.println("========================");
    // A tara nao e persistida: apos cada reinicio e obrigatorio zerar com a
    // plataforma vazia antes de pesar ou calibrar.
    Serial.println("ATENCAO: mantenha a plataforma vazia. A tara de partida sera feita na primeira leitura de cada sensor.");
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
                    // Zera a soma a cada nova linha recebida
                    pesoAtual = 0.0f;

                    Serial.print(sensores[i].prefixo + " RAW: ");
                    Serial.print(sensores[i].sensor.getRaw(), 3);

                    Serial.print(" | " + sensores[i].prefixo + " KG: ");
                    Serial.print(sensores[i].sensor.getKg(), 3);

                    // Acumula o valor em kg para o total
                    pesoAtual += sensores[i].sensor.getKg();

                    // Imprime o total acumulado até o momento
                    Serial.print(" | Total KG: ");
                    Serial.print(pesoAtual, 3);
                    // O valor bruto carrega o offset mecanico da plataforma:
                    // sem tara o peso exibido nao tem significado.
                    if (i < 4 && !sensorTarado[i])
                    {
                        Serial.print(" [SEM TARA]");
                    }
                    if (!sensores[i].sensor.estavel())
                    {
                        Serial.print(" [INSTAVEL]");
                    }
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
        // TARA (t1-t4)
        //////////////////////////////////////////////////////////////////////////

        if (comando.startsWith("t") || comando.startsWith("T"))
        {
            int selectedSensorIndex = comando.substring(1).toInt() - 1;
            if (selectedSensorIndex >= 0 && selectedSensorIndex < numSensores)
            {
                if (sensores[selectedSensorIndex].sensor.tare())
                {
                    sensorIndex[selectedSensorIndex] = selectedSensorIndex;
                    Serial.print("Sensor ");
                    Serial.print(selectedSensorIndex + 1);
                    Serial.println(" zerado.");
                }
            }
            else
            {
                Serial.println("ERRO: Sensor invalido (use t1-t4)");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // FATOR DE ESCALA (f[n] [fator] ou g[n])
        //////////////////////////////////////////////////////////////////////////

        else if (comando.startsWith("f") || comando.startsWith("F"))
        {
            // Formato "f[n] [fator]"
            int espacoIndex = comando.indexOf(' ');
            if (espacoIndex != -1)
            {
                int selectedSensorIndex = comando.substring(1, espacoIndex).toInt() - 1;
                float novoFatorValor = comando.substring(espacoIndex + 1).toFloat();

                if (selectedSensorIndex >= 0 && selectedSensorIndex < numSensores)
                {
                    sensores[selectedSensorIndex].sensor.setScale(novoFatorValor);
                    sensorIndex[selectedSensorIndex] = selectedSensorIndex;
                    novoFator[selectedSensorIndex] = novoFatorValor;
                    Serial.print("Sensor ");
                    Serial.print(selectedSensorIndex + 1);
                    Serial.print(" fator definido: ");
                    Serial.println(novoFatorValor, 8);
                }
                else
                {
                    Serial.println("ERRO: Sensor invalido (use f[n] [fator])");
                }
            }
        }
        else if (comando.startsWith("g") || comando.startsWith("G"))
        {
            int selectedSensorIndex = comando.substring(1).toInt() - 1;
            if (selectedSensorIndex >= 0 && selectedSensorIndex < numSensores)
            {
                Serial.print("Sensor ");
                Serial.print(selectedSensorIndex + 1);
                Serial.print(" fator: ");
                Serial.println(sensores[selectedSensorIndex].sensor.getScale(), 8);
            }
            else
            {
                Serial.println("ERRO: Sensor invalido (use g[n])");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // CALIBRAÇÃO COM PESO PADRÃO (c1, c2, c3, c4)
        //////////////////////////////////////////////////////////////////////////

        else if (comando.length() == 2 && (comando.startsWith("c") || comando.startsWith("C")))
        {
            int selectedSensorIndex = comando.substring(1).toInt() - 1;
            if (selectedSensorIndex >= 0 && selectedSensorIndex < numSensores)
            {
                Serial.println("--------------------------------");
                Serial.print("COLOQUE O PESO DE CALIBRACAO NO SENSOR ");
                Serial.println(selectedSensorIndex + 1);
                Serial.println("--------------------------------");

                delay(3000);

                sensores[selectedSensorIndex].sensor.calibra(pesoCalibracao1);
            }
            else
            {
                Serial.println("ERRO: Sensor invalido (use c1-c4)");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // CALIBRAÇÃO COM PESO INFORMADO (c1 5000, c2 5000, ...)
        //////////////////////////////////////////////////////////////////////////

        else if (comando.startsWith("c") || comando.startsWith("C"))
        {
            // Espera formato "c[n] [peso]"
            int espacoIndex = comando.indexOf(' ');
            if (espacoIndex != -1)
            {
                int selectedSensorIndex = comando.substring(1, espacoIndex).toInt() - 1;
                float pesoConhecido = comando.substring(espacoIndex + 1).toFloat();

                if (selectedSensorIndex >= 0 && selectedSensorIndex < numSensores && pesoConhecido > 0)
                {
                    Serial.println("--------------------------------");
                    Serial.print("AGUARDE ESTABILIZAR SENSOR ");
                    Serial.println(selectedSensorIndex + 1);
                    Serial.println("--------------------------------");

                    delay(3000);

                    sensores[selectedSensorIndex].sensor.calibra(pesoConhecido);
                }
                else
                {
                    Serial.println("ERRO: Formato invalido ou peso/sensor invalido (use c[n] [peso])");
                }
            }
            else
            {
                Serial.println("ERRO: Formato invalido (use c[n] [peso])");
            }
        }
    }
}