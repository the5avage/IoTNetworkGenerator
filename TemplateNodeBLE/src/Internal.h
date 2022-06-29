#pragma once
#include <BLEUUID.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>

extern BLEUUID serviceUUID;
extern BLEClient *bleClient;
extern BLEAdvertisedDevice* device;
extern bool connectionReady;

bool loadCharacteristics(BLERemoteService* service);


