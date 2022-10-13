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
        auto vec = Sender::testVector.get();
        if (vec.has_value())
        {
            Serial.print("value of  testvector [");
            for (auto i: vec.value())
            {
                Serial.print(i);
                Serial.print(",");
            }
            Serial.println("]");
        }
        else
        {
            Serial.println("Error receiving testvector");
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

void log(const char* message, Loglevel::Loglevel loglevel)
{
    if (loglevel >= Loglevel::error)
    {
        Serial.print("Log: ");
        Serial.println(message);
    }
}