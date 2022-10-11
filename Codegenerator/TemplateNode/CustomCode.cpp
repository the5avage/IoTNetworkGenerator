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


{% for fun in thisNode.get('functions', []) %}
{% set paramDecl = [] %}
    {% for p in fun.get('params', []) %}
        {% do paramDecl.append(p.type + " " + p.name) %}
    {% endfor %}
{{fun.returnType}} {{thisNode.name}}::{{fun.name}}({{paramDecl|join(', ')}})
{
}
{% endfor %}

void log(const char* message, Loglevel::Loglevel loglevel)
{
}