#include "bms_protection.h"

bms_protection_status_t bms_protection_evaluate(
    const bms_measurements_t *measurements,
    const bms_limits_t *limits)
{
    if (measurements == 0 || limits == 0)
    {
        return BMS_PROTECTION_INVALID_MEASUREMENT;
    }

    if (bms_measurements_validate(measurements) != 0)
    {
        return BMS_PROTECTION_INVALID_MEASUREMENT;
    }

    if (measurements->voltage.value > limits->max_voltage)
    {
        return BMS_PROTECTION_OVER_VOLTAGE;
    }

    if (measurements->voltage.value < limits->min_voltage)
    {
        return BMS_PROTECTION_UNDER_VOLTAGE;
    }

    if (measurements->current.value > limits->max_current)
    {
        return BMS_PROTECTION_OVER_CURRENT;
    }

    if (measurements->temperature.value > limits->max_temperature)
    {
        return BMS_PROTECTION_OVER_TEMPERATURE;
    }

    if (measurements->temperature.value < limits->min_temperature)
    {
        return BMS_PROTECTION_UNDER_TEMPERATURE;
    }

    return BMS_PROTECTION_NORMAL;
}
