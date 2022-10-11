#include "RemoteValues.h"
#include "RemoteValue.h"
#include "ComposedAttribute.h"

{% for node in otherNodes %}
namespace {{node.name}}
{
    {% for v in node.variables %}
        {% if v.composed is defined %}
ComposedAttribute<{{v.type}}> {{v.name}};
        {% else %}
RemoteValueReadOnly<{{v.type}}> {{v.name}};
        {% endif %}
    {% endfor %}

    {% for fun in node.get('functions', []) %}
        {% if fun.returnType is defined %}
RemoteFunction<{{ ([fun.returnType] + fun.get('params', [])|map(attribute='type')|list) |join(',') }}> {{fun.name}};
        {% else %}
RemoteFunctionVoid<{{ fun.get('params', [])|map(attribute='type')|join(',') }}> {{fun.name}};
        {% endif %}
    {% endfor %}
}

{% endfor %}

namespace {{thisNode.name}}
{
{% for v in thisNode.variables %}
    {% if v.composed is defined %}
ComposedAttribute<{{v.type}}> {{v.name}};
    {% else %}
RemoteValue<{{v.type}}> {{v.name}};
    {% endif %}
{% endfor %}
}

