#pragma once

#include <BLERemoteCharacteristic.h>

template<typename T>
class RemoteValueReadOnly
{
protected:
    BLERemoteCharacteristic* characteristic = nullptr;
public:
    bool get(T& value)
    {
        std::string rawValue = characteristic->readValue();
        if (rawValue.length() == sizeof(T)) {
            value = *(T*)(rawValue.data());
            return true;
        }
        return false;
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
            this->characteristic->writeValue((uint8_t*)&value, sizeof(T));
        }
    }

    RemoteValue(BLERemoteCharacteristic* characteristic) : RemoteValueReadOnly<T>(characteristic) {}
    RemoteValue() = default;
};
