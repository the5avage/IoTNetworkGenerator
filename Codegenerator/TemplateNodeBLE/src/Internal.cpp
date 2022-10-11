#include "Internal.h"
#include "RemoteValues.h"

BLEClient *bleClient = nullptr;
BLEAdvertisedDevice* device = nullptr;
bool connectionReady = false;
BLEUUID serviceUUID("{{service_uuid}}");
TaskBuffer taskBuffer;
std::vector<uint8_t> nodeUUID{ {{thisNode.uuid.bytes|join(',')}} };

{% for fun in thisNode.functions %}
BLERemoteCharacteristic* return_{{fun.name}};
{% endfor %}

bool loadCharacteristics(BLERemoteService* service)
{
    BLERemoteCharacteristic* tmpCharacteristic;

{% for node in otherNodes %}
    {% for v in node.variables %}
        {% if v.composed is defined %}
            {% for uuid in v.composed.uuids %}
    tmpCharacteristic = service->getCharacteristic("{{uuid}}");
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    {{node.name}}::{{v.name}}.characteristics.push_back(tmpCharacteristic);
            {% endfor %}
    {{node.name}}::{{v.name}}.attributeSize = {{v.composed.size}};
    {{node.name}}::{{v.name}}.numberOfAttributes = {{v.composed.length}};
    {{node.name}}::{{v.name}}.characteristics[0]->registerForNotify(notifyComposedAttribute<{{v.type}}, &{{node.name}}::{{v.name}}>);
        {% else %}
    tmpCharacteristic = service->getCharacteristic("{{v.uuid}}");
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    {{node.name}}::{{v.name}} = RemoteValueReadOnly<{{v.type}}>(tmpCharacteristic);
            {% if v.isObserved is defined %}
    tmpCharacteristic->registerForNotify(notifyCallback<{{v.type}}, {{node.name}}::onChange_{{v.name}}>);
            {% endif %}
        {% endif %}

    tmpCharacteristic = service->getCharacteristic("{{v.uuid}}");
    {% endfor %}

    {% for fun in node.functions %}
    tmpCharacteristic = service->getCharacteristic("{{fun.call_uuid}}");
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    {{node.name}}::{{fun.name}}.characteristic = tmpCharacteristic;
    {{node.name}}::{{fun.name}}.callTag.calleeUUID = nodeUUID;

    tmpCharacteristic = service->getCharacteristic("{{fun.return_uuid}}");
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    tmpCharacteristic->registerForNotify(notifyReturnCallback<
        decltype({{node.name}}::{{fun.name}}),
        &{{node.name}}::{{fun.name}}>);
    {% endfor %}
{% endfor %}

{% for v in thisNode.variables %}
    {% if v.composed is defined %}
        {% for uuid in v.composed.uuids %}
    tmpCharacteristic = service->getCharacteristic("{{uuid}}");
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    {{thisNode.name}}::{{v.name}}.characteristics.push_back(tmpCharacteristic);
        {% endfor %}
    {{thisNode.name}}::{{v.name}}.attributeSize = {{v.composed.size}};
    {{thisNode.name}}::{{v.name}}.numberOfAttributes = {{v.composed.length}};
    {{thisNode.name}}::{{v.name}}.characteristics[0]->registerForNotify(notifyComposedAttribute<{{v.type}}, &{{thisNode.name}}::{{v.name}}>);
    {% else %}
    tmpCharacteristic = service->getCharacteristic("{{v.uuid}}");
    if (tmpCharacteristic == nullptr)
    {
        return false;
    }
    {{thisNode.name}}::{{v.name}} = RemoteValue<{{v.type}}>(tmpCharacteristic);
    {% endif %}
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

void TaskBuffer::addTask(std::function<void(void)> task)
{
    while (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
        ;

    tasks.push_back(task);
    xSemaphoreGive(semaphore);
}

void TaskBuffer::executeTasks()
{
    if (xSemaphoreTake(semaphore, (TickType_t) portTICK_PERIOD_MS * 5000) != pdTRUE)
    {
        return;
    }
    for (auto& e: tasks)
    {
        e();
    }
    tasks.clear();
    xSemaphoreGive(semaphore);
}