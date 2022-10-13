#include <Arduino.h>
#include "src/GenCode.h"
#include "CustomCode.h"

void Setup()
{
    Serial.begin(115200);
    Serial.println("Setup finished");
}

std::vector<int> counts;
int count = 0;

void Loop()
{
    if (isConnected())
    {
        counts.push_back(count++);
        testVector.set(counts);
        Serial.println("Set testvector");
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
    if (loglevel >= Loglevel::debug)
    {
        Serial.print("Log: ");
        Serial.println(message);
    }
}