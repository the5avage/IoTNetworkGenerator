#pragma once
#include <PubSubClient.h>
#include <BLERemoteCharacteristic.h>
#include "Internal.h"

template<typename T>
class RemoteValueReadOnly
{
    friend void updateValues(char* topic, byte* message, unsigned int length);
protected:
    T cachedValue;
    bool hasValue = false;
public:
    bool get(T& value)
    {
        if (hasValue)
        {
            value = cachedValue;
            return true;
        }
        return false;
    }

    RemoteValueReadOnly() = default;
};


template<typename T>
class RemoteValue : public RemoteValueReadOnly<T>
{
PubSubClient* client;
const char* topic;
public:
    void set(T value)
    {
        client->publish(topic, (const uint8_t *)&value, (unsigned int) sizeof(T), true);
    }

    RemoteValue(PubSubClient* client, const char* topic) :
        RemoteValueReadOnly<T>(), client(client), topic(topic) {}
    RemoteValue() = default;
};
