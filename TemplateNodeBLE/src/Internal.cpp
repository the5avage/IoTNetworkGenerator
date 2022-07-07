#include "Internal.h"
#include "RemoteValues.h"

BLEClient *bleClient = nullptr;
BLEAdvertisedDevice* device = nullptr;
bool connectionReady = false;
BLEUUID serviceUUID("{{service_uuid}}");

bool loadCharacteristics(BLERemoteService* service)
{
    BLERemoteCharacteristic* tmpCharacteristic;

{% for node in otherNodes %}
    {% for v in node.variables %}
    tmpCharacteristic = service->getCharacteristic("{{v.uuid}}");
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    {{node.name}}::{{v.name}} = RemoteValueReadOnly<{{v.type}}>(tmpCharacteristic);
    {% endfor %}
{% endfor %}

{% for v in thisNode.variables %}
    tmpCharacteristic = service->getCharacteristic("{{v.uuid}}");
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    {{thisNode.name}}::{{v.name}} = RemoteValue<{{v.type}}>(tmpCharacteristic);
{% endfor %}
    return true;
}
