#include <Arduino.h>
#include "src/GenCode.h"
#include "CustomCode.h"
#include "Adafruit_MCP9808.h"
#include <cmath>

Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
float lastSendedTemp = -275.0f;

void Setup()
{
    Serial.begin(115200);
    if (!tempsensor.begin())
    {
        Serial.println("Couldn't find MCP9808!");
    }
    Serial.println("Setup finished");
}

void Loop()
{
    if (isConnected())
    {
        float currentTemp = tempsensor.readTempC();
        if (abs(currentTemp - lastSendedTemp) > 0.1)
        {
            lastSendedTemp = currentTemp;
            Serial.print("New temperature is: ");
            Serial.println(currentTemp);
            temperature.set(currentTemp);
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
