#pragma once
#include <PubSubClient.h>
#include <BLERemoteCharacteristic.h>
#include "Internal.h"
#include "optional.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Serialize.h"

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
class RemoteFunction
{
public:
    PubSubClient* client;
    const char* topic;
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
    nonstd::optional<R> result = nonstd::nullopt;
    nonstd::optional<R> operator()(Args... args)
    {
        result = nonstd::nullopt;
        std::vector<uint8_t> data = toBytes(args...);
        client->publish(topic, data.data(), data.size());
        xSemaphoreTake(semaphore, (TickType_t) 10);

        if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        {
            return nonstd::nullopt;
        }
        return result;
    }
    RemoteFunction() = default;
    RemoteFunction(PubSubClient* client, const char* topic) :
        client(client), topic(topic) {}
};

template <typename... Args>
class RemoteFunctionVoid
{
public:
    PubSubClient* client;
    const char* topic;
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
    bool result = false;
    bool operator()(Args... args)
    {
        result = false;
        std::vector<uint8_t> data = toBytes(args...);
        client->publish(topic, data.data(), data.size());
        xSemaphoreTake(semaphore, (TickType_t) 10);

        if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        {
            false;
        }
        return result;
    }
    RemoteFunctionVoid() = default;
    RemoteFunctionVoid(PubSubClient* client, const char* topic) :
        client(client), topic(topic) {}
};
