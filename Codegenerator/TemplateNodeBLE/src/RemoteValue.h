#pragma once

#include <BLERemoteCharacteristic.h>
#include "optional.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <functional>
#include "Serialize.h"

template<typename T>
class RemoteValueReadOnly
{
protected:
    BLERemoteCharacteristic* characteristic = nullptr;
public:
    bool get(T& value)
    {
        std::string rawValue = characteristic->readValue();
        if (rawValue.size() == 0)
        {
            return false;
        }
        value = deserialize<T>((uint8_t*)rawValue.data());
        return true;
    }

    RemoteValueReadOnly(BLERemoteCharacteristic* characteristic) : characteristic(characteristic) {}
    RemoteValueReadOnly() = default;
};

template<typename T>
class RemoteValue : public RemoteValueReadOnly<T>
{
public:
    void set(T value)
    {
        if (this->characteristic)
        {
            std::vector<uint8_t> rawData = toBytes(value);
            this->characteristic->writeValue(rawData.data(), rawData.size());
        }
    }

    RemoteValue(BLERemoteCharacteristic* characteristic) : RemoteValueReadOnly<T>(characteristic) {}
    RemoteValue() = default;
};

template <typename R, typename... Args>
class RemoteFunction
{
public:
    BLERemoteCharacteristic* characteristic = nullptr;
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
    nonstd::optional<R> result = nonstd::nullopt;

    nonstd::optional<R> operator()(Args... args)
    {
        result = nonstd::nullopt;
        std::vector<uint8_t> data = toBytes(args...);
        characteristic->writeValue(data.data(), data.size());
        xSemaphoreTake(semaphore, (TickType_t) 10);

        if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        {
            return nonstd::nullopt;
        }
        return result;
    }

    void receiveResult(R value)
    {
        result = value;
        xSemaphoreGive(semaphore);
    }

    RemoteFunction() = default;
};

template <typename... Args>
class RemoteFunctionVoid
{
public:
    BLERemoteCharacteristic* characteristic = nullptr;
    SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
    bool result = false;

    bool operator()(Args... args)
    {
        result = false;
        std::vector<uint8_t> data = toBytes(args...);
        characteristic->writeValue(data.data(), data.size());
        xSemaphoreTake(semaphore, (TickType_t) 10);

        if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        {
            false;
        }
        return result;
    }

    void receiveResult()
    {
        xSemaphoreGive(semaphore);
    }

    RemoteFunctionVoid() = default;
};
