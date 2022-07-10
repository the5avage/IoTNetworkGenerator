#include <Arduino.h>
#include "src/GenCode.h"
#include "CustomCode.h"

void Setup()
{
    Serial.begin(115200);
    Serial.println("Setup finished");
}

void Loop()
{
    if (isConnected())
    {
        float innenTemp;
        float aussenTemp;
        if (InsideTemperatureSensor::temperature.get(innenTemp) && 
            OutsideTemperatureSensor::temperature.get(aussenTemp))
        {
            Serial.print("Innentemperatur: ");
            Serial.print(innenTemp);
            Serial.print("Aussentemperatur: ");
            Serial.println(aussenTemp);
        }
        else
        {
            Serial.println("Error reading temperatures");
        }
    }
    else
    {
        Serial.println("No connection to server");
    }
    delay(1000);
}

void OnConnect()
{
    Serial.println("Connected to server");
}

void OnDisconnect()
{
    Serial.println("Disconnected from server");
}

void OutsideTemperatureSensor::onChange_temperature(float value)
{
    Serial.print("Outside temp changed to: ");
    Serial.println(value);
}

void InsideTemperatureSensor::onChange_temperature(float value)
{
    Serial.print("Outside temp changed to: ");
    Serial.println(value);
}
