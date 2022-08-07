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
        auto reply = StringTest::greet(std::string("Rene"));
        if (reply)
        {
            Serial.print("Success calling function. Reply: ");
            Serial.println(reply.value().c_str());

        }
        else
        {
            Serial.println("Error calling function");
        }
    }
    else
    {
        Serial.println("No connection to server");
    }
    std::vector<int> primes = {2, 3, 5, 7};
    testVector.set(primes);
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

std::vector<int> VectorTest::append42(std::vector<int> array)
{
    Serial.print("Append 42 to [");
    for (auto i: array)
    {
        Serial.print(i);
        Serial.print(",");
    }
    Serial.println("]");
    array.push_back(42);
    return array;
}
