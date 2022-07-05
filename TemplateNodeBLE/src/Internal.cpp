#include "Internal.h"
#include "RemoteValues.h"

BLEUUID serviceUUID("{{service_uuid}}");
{% for value in reads + writes %}
BLEUUID {{value.name}}UUID("{{value.uuid}}");
{% endfor %}
BLEClient *bleClient = nullptr;
BLEAdvertisedDevice* device = nullptr;
bool connectionReady = false;

bool loadCharacteristics(BLERemoteService* service)
{
    BLERemoteCharacteristic* tmpCharacteristic;

{% for value in reads %}
    tmpCharacteristic = service->getCharacteristic({{value.name}}UUID);
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    {{value.name}} = RemoteValueReadOnly<{{value.type}}>(tmpCharacteristic);
{% endfor %}

{% for value in writes %}
    tmpCharacteristic = service->getCharacteristic({{value.name}}UUID);
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    {{value.name}} = RemoteValue<{{value.type}}>(tmpCharacteristic);
{% endfor %}
    return true;
}
