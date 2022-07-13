#include "Internal.h"
#include "RemoteValues.h"

bool connectionReady = false;
const char *ssid = "{{communication_protocol.ssid}}";
const char *password = "{{communication_protocol.password}}";
const char *broker_address = "{{communication_protocol.broker_address}}";
const char *client_name = "{{thisNode.name}}";
int broker_port = {{communication_protocol.broker_port}};

void subscribeToTopics(PubSubClient* client)
{
{% for node in otherNodes + [thisNode] %}
    {% for v in node.variables %}
    client->subscribe("{{node.name}}/{{v.name}}");
    {% endfor %}
{% endfor %}
}

void initializeValues(PubSubClient* client)
{
{% for node in otherNodes + [thisNode] %}
    {% for v in node.variables %}
    {{node.name}}::{{v.name}} = RemoteValue<{{v.type}}>(client, "{{node.name}}/{{v.name}}");
    {% endfor %}
{% endfor %}
}

void updateValues(char* topic, byte* message, unsigned int length)
{
{% for node in otherNodes + [thisNode] %}
    {% for v in node.variables %}
    if (!strcmp(topic, "{{node.name}}/{{v.name}}"))
    {
        if (length == sizeof(float))
        {
            {{node.name}}::{{v.name}}.cachedValue = *({{v.type}}*)message;
            {{node.name}}::{{v.name}}.hasValue = true;
        {% if v.isObserved is defined %}
            {{node.name}}::onChange_{{v.name}}({{node.name}}::{{v.name}}.cachedValue);
        {% endif %}
        }
        return;
    }
    {% endfor %}
{% endfor %}
}

