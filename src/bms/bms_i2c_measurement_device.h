#ifndef BMS_I2C_MEASUREMENT_DEVICE_H
#define BMS_I2C_MEASUREMENT_DEVICE_H

#include <stdint.h>

#include "bms_measurement_device.h"
#include "bms_measurements.h"

/**
 * @brief Context for the project-defined reference I2C measurement device.
 */
typedef struct
{
    uint8_t dev_addr;      /**< 7-bit I2C device address */
    uint16_t timeout_ms;   /**< Reserved; driver currently uses its own timeout */
} bms_i2c_measurement_context_t;

/**
 * @brief Read measurements from the reference I2C device.
 *
 * Implements bms_measurement_device_read_fn.
 *
 * Protocol:
 *   device address: 0x40
 *   start register: 0x00
 *   burst length:   6 bytes
 *
 * @param measurements Destination measurement container.
 * @param context      bms_i2c_measurement_context_t.
 *
 * @return BMS_MEAS_DEVICE_OK on successful acquisition.
 * @return BMS_MEAS_DEVICE_ERROR on communication/acquisition failure.
 */
bms_measurement_device_status_t
bms_i2c_measurement_device_read(
    bms_measurements_t *measurements,
    void *context
);

#endif /* BMS_I2C_MEASUREMENT_DEVICE_H */
