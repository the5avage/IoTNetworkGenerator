#include "Internal.h"
#include "RemoteValues.h"

BLEUUID serviceUUID(CODEGEN_SERVICE_UUID);
CODEGEN_DEFINE_CHARACTERISTICS_UUID
BLEClient *bleClient = nullptr;
BLEAdvertisedDevice* device = nullptr;
bool connectionReady = false;

bool loadCharacteristics(BLERemoteService* service)
{
    BLERemoteCharacteristic* tmpCharacteristic;

    CODEGEN_LOAD_CHARACTERISTICS
}
