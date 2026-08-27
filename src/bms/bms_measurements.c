#include "bms_measurements.h"

#include <math.h>

void bms_measurements_init(bms_measurements_t *measurements)
{
    if (measurements == 0)
    {
        return;
    }

    measurements->voltage.value = 0.0f;
    measurements->voltage.status = BMS_MEAS_NOT_AVAILABLE;

    measurements->current.value = 0.0f;
    measurements->current.status = BMS_MEAS_NOT_AVAILABLE;

    measurements->temperature.value = 0.0f;
    measurements->temperature.status = BMS_MEAS_NOT_AVAILABLE;
}

int bms_measurements_validate(const bms_measurements_t *measurements)
{
    if (measurements == 0)
    {
        return -1;
    }

    if (measurements->voltage.status != BMS_MEAS_VALID ||
        measurements->current.status != BMS_MEAS_VALID ||
        measurements->temperature.status != BMS_MEAS_VALID)
    {
        return -1;
    }

    /*
     * A measurement marked VALID must also contain a finite
     * numerical value. NaN and +/-Inf are never acceptable
     * as valid safety-critical measurements.
     */
    if (!isfinite(measurements->voltage.value) ||
        !isfinite(measurements->current.value) ||
        !isfinite(measurements->temperature.value))
    {
        return -1;
    }

    return 0;
}