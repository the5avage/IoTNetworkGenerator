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
        std::vector<int> arg;
        arg.push_back(1);
        arg.push_back(5);
        arg.push_back(13);
        auto reply = VectorTest::append42(arg);
        if (reply)
        {
            Serial.println("Success calling function. Received:");

            for (auto i: reply.value())
            {
                Serial.print(i);
                Serial.print(",");
            }
            Serial.println("]");
        }
        else
        {
            Serial.println("Error calling function");
        }
        testString.set("Hello cruel world");

        auto vec = VectorTest::testVector.get();
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

std::string StringTest::greet(std::string forename)
{
    Serial.print("Say hello to: ");
    Serial.println(forename.c_str());
    return std::string("Hello, ") + forename;
}
