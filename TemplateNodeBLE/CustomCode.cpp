#include <Arduino.h>
#include "src/GenCode.h"
#include "CustomCode.h"

void Setup()
{
}

void Loop()
{
}

void OnConnect()
{
}

void OnDisconnect()
{
}

{% for node in otherNodes %}
    {% for v in node.variables %}
        {% if v.isObserved is defined %}
void {{node.name}}::onChange_{{v.name}}({{v.type}} value)
{
}

        {% endif %}
    {% endfor %}
{% endfor %}
