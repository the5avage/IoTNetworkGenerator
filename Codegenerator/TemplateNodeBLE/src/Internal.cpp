#include "Internal.h"
#include "RemoteValues.h"

BLEClient *bleClient = nullptr;
BLEAdvertisedDevice* device = nullptr;
bool connectionReady = false;
BLEUUID serviceUUID("{{service_uuid}}");
OutputBuffer outputBuffer;

{% for fun in thisNode.functions %}
BLERemoteCharacteristic* return_{{fun.name}};
{% endfor %}

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
        {% if v.isObserved is defined %}
    tmpCharacteristic->registerForNotify(notifyCallback<{{v.type}}, {{node.name}}::onChange_{{v.name}}>);
        {% endif %}
    {{node.name}}::{{v.name}} = RemoteValueReadOnly<{{v.type}}>(tmpCharacteristic);
    {% endfor %}

    {% for fun in node.functions %}
    tmpCharacteristic = service->getCharacteristic("{{fun.call_uuid}}");
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    {{node.name}}::{{fun.name}}.characteristic = tmpCharacteristic;

    tmpCharacteristic = service->getCharacteristic("{{fun.return_uuid}}");
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
        {% if fun.returnType is defined %}
    tmpCharacteristic->registerForNotify(notifyReturnCallback<
        {{fun.returnType}},
        decltype({{node.name}}::{{fun.name}}),
        &{{node.name}}::{{fun.name}}>);
        {% else %}
    tmpCharacteristic->registerForNotify(notifyReturnVoidCallback<
        decltype({{node.name}}::{{fun.name}}),
        &{{node.name}}::{{fun.name}}>);
        {% endif %}
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

{% for fun in thisNode.functions %}
    return_{{fun.name}} = service->getCharacteristic("{{fun.return_uuid}}");
    tmpCharacteristic = service->getCharacteristic("{{fun.call_uuid}}");
    {% if fun.returnType is defined %}
    tmpCharacteristic->registerForNotify(notifyCallCallback<&return_{{fun.name}}, decltype({{thisNode.name}}::{{fun.name}}),
    {{ ([thisNode.name+'::'+fun.name] + fun.get('params', [])|map(attribute='type')|list) |join(',') }}>);
    {% else %}
    tmpCharacteristic->registerForNotify(notifyCallVoidCallback<&return_{{fun.name}}, decltype({{thisNode.name}}::{{fun.name}}),
    {{ ([thisNode.name+'::'+fun.name] + fun.get('params', [])|map(attribute='type')|list) |join(',') }}>);
    {% endif %}
{% endfor %}
    return true;
}

void OutputBuffer::addData(BLERemoteCharacteristic* target, std::vector<uint8_t>& data)
{
    while (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        ;

    buffer.push_back(OutputBufferEntry{target, data});
    xSemaphoreGive(semaphore);
}

void OutputBuffer::sendData()
{
    if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
    {
        return;
    }
    for (auto& e: buffer)
    {
        e.target->writeValue(e.data.data(), e.data.size());
    }
    buffer.clear();
    xSemaphoreGive(semaphore);
}