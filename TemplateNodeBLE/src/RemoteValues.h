#pragma once

#include "RemoteValue.h"


{% for node in otherNodes %}
namespace {{node.name}}
{
    {% for v in node.variables %}
extern RemoteValueReadOnly<{{v.type}}> {{v.name}};
        {% if v.isObserved is defined %}
void onChange_{{v.name}}({{v.type}});
        {% endif %}
    {% endfor %}
}

{% endfor %}

namespace {{thisNode.name}}
{
{% for v in thisNode.variables %}
extern RemoteValue<{{v.type}}> {{v.name}};
{% endfor %}
}
