#include "bms_measurement_device.h"

void bms_measurement_device_init(
    bms_measurement_device_t *device,
    bms_measurement_device_read_fn read_fn,
    void *context)
{
    if (device == 0)
    {
        return;
    }

    device->read = read_fn;
    device->context = context;
}

bms_measurement_device_status_t bms_measurement_device_read(
    const bms_measurement_device_t *device,
    bms_measurements_t *measurements)
{
    if (device == 0 || measurements == 0 || device->read == 0)
    {
        return BMS_MEAS_DEVICE_ERROR;
    }

    return device->read(measurements, device->context);
}
