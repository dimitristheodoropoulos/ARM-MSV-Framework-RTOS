#include "bms_measurements.h"

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

    if (measurements->voltage.status != BMS_MEAS_VALID)
    {
        return -1;
    }

    if (measurements->current.status != BMS_MEAS_VALID)
    {
        return -1;
    }

    if (measurements->temperature.status != BMS_MEAS_VALID)
    {
        return -1;
    }

    return 0;
}