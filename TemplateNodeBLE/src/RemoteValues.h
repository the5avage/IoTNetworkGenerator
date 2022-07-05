#pragma once

#include "RemoteValue.h"

{% for value in reads %}
extern RemoteValueReadOnly<{{value.type}}> {{value.name}};
{% endfor %}
{% for value in writes %}
extern RemoteValue<{{value.type}}> {{value.name}};
{% endfor %}