#include "RemoteValues.h"
#include "RemoteValue.h"

{% for node in otherNodes %}
namespace {{node.name}}
{
    {% for v in node.variables %}
RemoteValueReadOnly<{{v.type}}> {{v.name}};
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
RemoteValue<{{v.type}}> {{v.name}};
{% endfor %}
}

