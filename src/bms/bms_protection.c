#include "bms_protection.h"

bms_fault_mask_t bms_protection_evaluate_faults(
    const bms_measurements_t *measurements,
    const bms_limits_t *limits)
{
    bms_fault_mask_t faults = BMS_FAULT_MASK_NONE;

    if (measurements == 0 || limits == 0)
    {
        return BMS_FAULT_MASK_NONE;
    }

    if (bms_measurements_validate(measurements) != 0)
    {
        return BMS_FAULT_MASK_NONE;
    }

    if (measurements->voltage.value > limits->max_voltage)
    {
        faults |= BMS_FAULT_MASK_OVER_VOLTAGE;
    }

    if (measurements->voltage.value < limits->min_voltage)
    {
        faults |= BMS_FAULT_MASK_UNDER_VOLTAGE;
    }

    if (measurements->current.value > limits->max_current)
    {
        faults |= BMS_FAULT_MASK_OVER_CURRENT;
    }

    if (measurements->temperature.value > limits->max_temperature)
    {
        faults |= BMS_FAULT_MASK_OVER_TEMPERATURE;
    }

    if (measurements->temperature.value < limits->min_temperature)
    {
        faults |= BMS_FAULT_MASK_UNDER_TEMPERATURE;
    }

    return faults;
}

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

    /*
     * Preserve the established deterministic priority order.
     * The multi-fault API above records all active conditions.
     */
    {
        bms_fault_mask_t faults =
            bms_protection_evaluate_faults(measurements, limits);

        if ((faults & BMS_FAULT_MASK_OVER_VOLTAGE) != 0u)
        {
            return BMS_PROTECTION_OVER_VOLTAGE;
        }

        if ((faults & BMS_FAULT_MASK_UNDER_VOLTAGE) != 0u)
        {
            return BMS_PROTECTION_UNDER_VOLTAGE;
        }

        if ((faults & BMS_FAULT_MASK_OVER_CURRENT) != 0u)
        {
            return BMS_PROTECTION_OVER_CURRENT;
        }

        if ((faults & BMS_FAULT_MASK_OVER_TEMPERATURE) != 0u)
        {
            return BMS_PROTECTION_OVER_TEMPERATURE;
        }

        if ((faults & BMS_FAULT_MASK_UNDER_TEMPERATURE) != 0u)
        {
            return BMS_PROTECTION_UNDER_TEMPERATURE;
        }
    }

    return BMS_PROTECTION_NORMAL;
}
