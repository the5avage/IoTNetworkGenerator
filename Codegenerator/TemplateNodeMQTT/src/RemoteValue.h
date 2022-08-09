#pragma once
#include <PubSubClient.h>
#include <BLERemoteCharacteristic.h>
#include "Internal.h"
#include "optional.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Serialize.h"
#include "RemoteFunctionAbstract.h"

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
        std::vector<uint8_t> data = toBytes(value);
        client->publish(topic, data.data(), data.size(), true);
    }

    RemoteValue(PubSubClient* client, const char* topic) :
        RemoteValueReadOnly<T>(), client(client), topic(topic) {}
    RemoteValue() = default;
};

template <typename R, typename... Args>
class RemoteFunction : public RemoteFunctionAbstract<R, Args...>
{
public:
    PubSubClient* client;
    const char* topic;

    virtual void sendData(std::vector<uint8_t>& data)
    {
        client->publish(topic, data.data(), data.size());
    }

    RemoteFunction() = default;
};

template <typename... Args>
class RemoteFunctionVoid : public RemoteFunctionVoidAbstract<Args...>
{
public:
    PubSubClient* client;
    const char* topic;

    virtual void sendData(std::vector<uint8_t>& data)
    {
        client->publish(topic, data.data(), data.size());
    }

    RemoteFunctionVoid() = default;
};
