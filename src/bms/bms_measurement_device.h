#ifndef BMS_MEASUREMENT_DEVICE_H
#define BMS_MEASUREMENT_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bms_measurements.h"

/**
 * @brief Result of a measurement transaction.
 *
 * Communication errors are represented explicitly so that the
 * BMS measurement layer can distinguish successful acquisition
 * from failed communication.
 */
typedef enum
{
    BMS_MEASUREMENT_OK = 0,
    BMS_MEASUREMENT_ERR_INVALID,
    BMS_MEASUREMENT_ERR_NACK,
    BMS_MEASUREMENT_ERR_TIMEOUT,
    BMS_MEASUREMENT_ERR_BUS,
    BMS_MEASUREMENT_ERR_ARBITRATION
} bms_measurement_result_t;

/**
 * @brief Abstract measurement-device read operation.
 *
 * The callback represents the boundary between the BMS
 * measurement layer and the underlying measurement transport.
 *
 * @param context     Opaque caller-owned device/transport context.
 * @param measurement Destination for the acquired measurement.
 *
 * @return BMS_MEASUREMENT_OK on successful acquisition.
 * @return Appropriate error code on transaction failure.
 */
typedef bms_measurement_result_t (*bms_measurement_read_fn)(
    void *context,
    bms_measurement_t *measurement);

/**
 * @brief Abstract BMS measurement device.
 *
 * The device does not own the context. The caller is responsible
 * for the lifetime of the context and the callback implementation.
 */
typedef struct
{
    void *context;
    bms_measurement_read_fn read;
} bms_measurement_device_t;

/**
 * @brief Read one measurement through the abstract device interface.
 *
 * A successful transaction shall produce BMS_MEAS_VALID.
 * A failed transaction shall produce BMS_MEAS_INVALID and shall
 * return the corresponding transaction error.
 *
 * @param device      Measurement-device abstraction.
 * @param measurement Destination measurement.
 *
 * @return Transaction result.
 */
bms_measurement_result_t bms_measurement_device_read(
    bms_measurement_device_t *device,
    bms_measurement_t *measurement);

#ifdef __cplusplus
}
#endif

#endif /* BMS_MEASUREMENT_DEVICE_H */
