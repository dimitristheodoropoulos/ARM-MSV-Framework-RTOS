#include "bms_limits.h"

#include <math.h>

int bms_limits_validate(const bms_limits_t *limits)
{
    if (limits == 0)
    {
        return -1;
    }

    if (!isfinite(limits->min_voltage) ||
        !isfinite(limits->max_voltage) ||
        !isfinite(limits->max_current) ||
        !isfinite(limits->min_temperature) ||
        !isfinite(limits->max_temperature))
    {
        return -1;
    }

    if (limits->min_voltage >= limits->max_voltage)
    {
        return -1;
    }

    if (limits->min_temperature >= limits->max_temperature)
    {
        return -1;
    }

    if (limits->max_current <= 0.0f)
    {
        return -1;
    }

    return 0;
}
