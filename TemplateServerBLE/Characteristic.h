#pragma once

#include <BLECharacteristic.h>

template<typename T>
class Characteristic : public BLECharacteristic
{
public:
    void set(T value)
    {
        setValue((uint8_t*)&value, sizeof(T));
    }

    T get()
    {
        return *(T*)getData();
    }

    Characteristic(const char* uuid, uint32_t properties) : BLECharacteristic(uuid, properties) {}
};

class CharacteristicCallback : public BLECharacteristicCallbacks
{
	void onWrite(BLECharacteristic* pCharacteristic) override
    {
        pCharacteristic->notify();
    }
};
