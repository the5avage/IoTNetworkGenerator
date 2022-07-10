#include "RemoteValues.h"
#include "RemoteValue.h"

{% for node in otherNodes %}
namespace {{node.name}}
{
    {% for v in node.variables %}
RemoteValueReadOnly<{{v.type}}> {{v.name}};
    {% endfor %}
}

{% endfor %}

namespace {{thisNode.name}}
{
{% for v in thisNode.variables %}
RemoteValue<{{v.type}}> {{v.name}};
{% endfor %}
}
