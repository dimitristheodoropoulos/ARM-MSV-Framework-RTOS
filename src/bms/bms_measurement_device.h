#ifndef BMS_MEASUREMENT_DEVICE_H
#define BMS_MEASUREMENT_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bms_measurements.h"

typedef enum
{
    BMS_MEAS_DEVICE_OK = 0,
    BMS_MEAS_DEVICE_ERROR
} bms_measurement_device_status_t;

/**
 * @brief Measurement device read callback.
 *
 * The callback represents the hardware-specific acquisition operation.
 * The BMS layer does not depend on the underlying transport.
 *
 * @param measurements Destination for acquired measurements.
 * @param context      Device-specific context.
 *
 * @return BMS_MEAS_DEVICE_OK on successful acquisition.
 * @return BMS_MEAS_DEVICE_ERROR on communication/acquisition failure.
 */
typedef bms_measurement_device_status_t
(*bms_measurement_device_read_fn)(
    bms_measurements_t *measurements,
    void *context
);

/**
 * @brief Measurement device abstraction.
 */
typedef struct
{
    bms_measurement_device_read_fn read;
    void *context;
} bms_measurement_device_t;

/**
 * @brief Initialize a measurement device abstraction.
 *
 * @param device  Device abstraction to initialize.
 * @param read_fn Hardware-specific read callback.
 * @param context Hardware-specific context.
 */
void bms_measurement_device_init(
    bms_measurement_device_t *device,
    bms_measurement_device_read_fn read_fn,
    void *context
);

/**
 * @brief Read measurements through the device abstraction.
 *
 * @param device       Measurement device.
 * @param measurements Destination for acquired measurements.
 *
 * @return BMS_MEAS_DEVICE_OK on successful acquisition.
 * @return BMS_MEAS_DEVICE_ERROR on invalid device, invalid destination,
 *         missing callback, or communication/acquisition failure.
 */
bms_measurement_device_status_t bms_measurement_device_read(
    const bms_measurement_device_t *device,
    bms_measurements_t *measurements
);

#ifdef __cplusplus
}
#endif

#endif /* BMS_MEASUREMENT_DEVICE_H */
