#pragma once

#include "RemoteValue.h"
#include "ComposedAttribute.h"


{% for node in otherNodes %}
namespace {{node.name}}
{
    {% for v in node.variables %}
        {% if v.composed is defined %}
extern ComposedAttribute<{{v.type}}> {{v.name}};
        {% else %}
extern RemoteValueReadOnly<{{v.type}}> {{v.name}};
        {% endif %}
        {% if v.isObserved is defined %}
void onChange_{{v.name}}({{v.type}});
        {% endif %}
    {% endfor %}

    {% for fun in node.get('functions', []) %}
        {% if fun.returnType is defined %}
extern RemoteFunction<{{ ([fun.returnType] + fun.get('params', [])|map(attribute='type')|list) |join(',') }}> {{fun.name}};
        {% else %}
extern RemoteFunctionVoid<{{ fun.get('params', [])|map(attribute='type')|join(',') }}> {{fun.name}};
        {% endif %}
    {% endfor %}
}

{% endfor %}

namespace {{thisNode.name}}
{
{% for v in thisNode.variables %}
    {% if v.composed is defined %}
extern ComposedAttribute<{{v.type}}> {{v.name}};
    {% else %}
extern RemoteValue<{{v.type}}> {{v.name}};
    {% endif %}
{% endfor %}

{% for fun in thisNode.get('functions', []) %}
{% set paramDecl = [] %}
    {% for p in fun.get('params', []) %}
        {% do paramDecl.append(p.type + " " + p.name) %}
    {% endfor %}
{{fun.get('returnType', 'void')}} {{fun.name}}({{paramDecl|join(", ")}});
{% endfor %}

}
