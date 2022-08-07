#include "Internal.h"
#include "RemoteValues.h"
#include "Serialize.h"

bool connectionReady = false;
const char *ssid = "{{thisNode.communication_protocol.ssid}}";
const char *password = "{{thisNode.communication_protocol.password}}";
const char *broker_address = "{{mqtt_broker.broker_address}}";
const char *client_name = "{{thisNode.name}}";
int broker_port = {{mqtt_broker.broker_port}};

void subscribeToTopics(PubSubClient* client)
{
{% for node in otherNodes + [thisNode] %}
    {% for v in node.variables %}
    client->subscribe("{{node.name}}/{{v.name}}");
    {% endfor %}

    {% for fun in node.functions %}
    client->subscribe("{{node.name}}/__return/{{fun.name}}");
    {% endfor %}

{% endfor %}

{% for fun in thisNode.functions %}
    client->subscribe("{{thisNode.name}}/__call/{{fun.name}}");
{% endfor %}
}

void initializeValues(PubSubClient* client)
{
{% for node in otherNodes + [thisNode] %}
    {% for v in node.variables %}
    {{node.name}}::{{v.name}} = RemoteValue<{{v.type}}>(client, "{{node.name}}/{{v.name}}");
    {% endfor %}
{% endfor %}

{% for node in otherNodes %}
    {% for fun in node.functions %}
    {{node.name}}::{{fun.name}}.client = client;
    {{node.name}}::{{fun.name}}.topic = "{{node.name}}/__call/{{fun.name}}";
    {% endfor %}
{% endfor %}
}

void updateValues(char* topic, byte* message, unsigned int length)
{
{% for node in otherNodes + [thisNode] %}
    {% for v in node.variables %}
    if (!strcmp(topic, "{{node.name}}/{{v.name}}"))
    {
        {{node.name}}::{{v.name}}.cachedValue = std::get<0>(deserialize<{{v.type}}>(message));
        {{node.name}}::{{v.name}}.hasValue = true;
        {% if v.isObserved is defined %}
        {{node.name}}::onChange_{{v.name}}({{node.name}}::{{v.name}}.cachedValue);
        {% endif %}
        return;
    }
    {% endfor %}

{% endfor %}

{% for node in otherNodes %}
    {% for fun in node.functions %}
    if (!strcmp(topic, "{{node.name}}/__return/{{fun.name}}"))
    {
        {% if fun.returnType is defined %}
        {{node.name}}::{{fun.name}}.result = std::get<0>(deserialize<{{fun.returnType}}>(message));
        {% else %}
        {{node.name}}::{{fun.name}}.result = true;
        {% endif %}
        xSemaphoreGive({{node.name}}::{{fun.name}}.semaphore);
    }
    {% endfor %}
{% endfor %}

{% for fun in thisNode.functions %}
    if (!strcmp(topic, "{{thisNode.name}}/__call/{{fun.name}}"))
    {
    {% for param in fun.get("params", []) %}
        {{param.type}} {{param.name}} = deserialize(message, static_cast<{{param.type}}*>(0));
    {% endfor %}
    {% if fun.returnType is defined %}
        auto result = {{thisNode.name}}::{{fun.name}}({{fun.get('params', [])|map(attribute='name')|join(',')}});
        std::vector<uint8_t> data = toBytes(result);
        client.publish("{{thisNode.name}}/__return/{{fun.name}}", data.data(), data.size());
    {% else %}
        {{thisNode.name}}::{{fun.name}}({{fun.get('params', [])|map(attribute='name')|join(',')}});
        client.publish("{{thisNode.name}}/__return/{{fun.name}}", "");
        return;
    {% endif %}
    }
{% endfor %}
}

