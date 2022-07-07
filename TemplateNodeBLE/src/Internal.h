#pragma once
#include <BLEUUID.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>

extern BLEUUID serviceUUID;
extern BLEClient *bleClient;
extern BLEAdvertisedDevice* device;
extern bool connectionReady;

bool loadCharacteristics(BLERemoteService* service);

template<typename T, void (*Fun)(T)>
void notifyCallback(BLERemoteCharacteristic* remoteCharacteristic, uint8_t* data, size_t length, bool isNotify)
{
    if (length == sizeof(T))
    {
        Fun(*(T*)data);
    }
}
