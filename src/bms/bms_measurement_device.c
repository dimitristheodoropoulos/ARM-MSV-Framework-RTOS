#include "bms_measurement_device.h"

bms_measurement_result_t bms_measurement_device_read(
    bms_measurement_device_t *device,
    bms_measurement_t *measurement)
{
    if (measurement == 0)
    {
        return BMS_MEASUREMENT_ERR_INVALID;
    }

    if (device == 0 || device->read == 0)
    {
        measurement->status = BMS_MEAS_INVALID;
        return BMS_MEASUREMENT_ERR_INVALID;
    }

    bms_measurement_result_t result =
        device->read(device->context, measurement);

    if (result != BMS_MEASUREMENT_OK)
    {
        measurement->status = BMS_MEAS_INVALID;
        return result;
    }

    if (measurement->status != BMS_MEAS_VALID)
    {
        measurement->status = BMS_MEAS_INVALID;
        return BMS_MEASUREMENT_ERR_INVALID;
    }

    return BMS_MEASUREMENT_OK;
}
