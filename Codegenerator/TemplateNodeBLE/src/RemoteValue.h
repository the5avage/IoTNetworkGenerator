#pragma once

#include <BLERemoteCharacteristic.h>
#include "optional.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <functional>
#include "Serialize.h"
#include "RemoteFunctionAbstract.h"

template<typename T>
class RemoteValueReadOnly
{
protected:
    BLERemoteCharacteristic* characteristic = nullptr;
public:
    nonstd::optional<T> get()
    {
        std::string rawValue = characteristic->readValue();
        if (rawValue.size() == 0)
        {
            return nonstd::nullopt;
        }
        uint8_t* data = (uint8_t*)rawValue.data();
        auto des = deserialize<T>(data, data + rawValue.size());
        if (!des.has_value())
        {
            return nonstd::nullopt;
        }
        return std::get<0>(des.value());
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
class RemoteFunction : public RemoteFunctionAbstract<R, Args...>
{
public:
    BLERemoteCharacteristic* characteristic = nullptr;

    virtual void sendData(std::vector<uint8_t>& data)
    {
        characteristic->writeValue(data.data(), data.size());
    }

    RemoteFunction() = default;
};

template <typename... Args>
class RemoteFunctionVoid : public RemoteFunctionVoidAbstract<Args...>
{
public:
    BLERemoteCharacteristic* characteristic = nullptr;

    virtual void sendData(std::vector<uint8_t>& data)
    {
        characteristic->writeValue(data.data(), data.size());
    }

    RemoteFunctionVoid() = default;
};
