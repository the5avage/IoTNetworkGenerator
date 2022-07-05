#include "RemoteValues.h"
#include "RemoteValue.h"

{% for value in reads %}
RemoteValueReadOnly<{{value.type}}> {{value.name}};
{% endfor %}
{% for value in writes %}
RemoteValue<{{value.type}}> {{value.name}};
{% endfor %}