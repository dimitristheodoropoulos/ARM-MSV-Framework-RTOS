#include "bms_i2c_measurement_device.h"

#include "drivers/i2c.h"

#include <stdint.h>

/* Project-defined reference device protocol. */
#define BMS_I2C_REF_START_REG  0x00U
#define BMS_I2C_REF_READ_LEN   6U

/* Reference protocol scaling. */
#define BMS_I2C_VOLTAGE_SCALE  1000.0f
#define BMS_I2C_CURRENT_SCALE  1000.0f
#define BMS_I2C_TEMP_SCALE       10.0f

bms_measurement_device_status_t
bms_i2c_measurement_device_read(
    bms_measurements_t *measurements,
    void *context)
{
    if (measurements == 0 || context == 0)
    {
        return BMS_MEAS_DEVICE_ERROR;
    }

    bms_i2c_measurement_context_t *ctx =
        (bms_i2c_measurement_context_t *)context;

    /*
     * Establish deterministic failure state before attempting I2C.
     */
    bms_measurements_init(measurements);

    uint8_t raw_data[BMS_I2C_REF_READ_LEN];

    int status = i2c_read_burst(
        ctx->dev_addr,
        BMS_I2C_REF_START_REG,
        raw_data,
        BMS_I2C_REF_READ_LEN
    );

    if (status != I2C_OK)
    {
        measurements->voltage.value = 0.0f;
        measurements->current.value = 0.0f;
        measurements->temperature.value = 0.0f;

        measurements->voltage.status = BMS_MEAS_INVALID;
        measurements->current.status = BMS_MEAS_INVALID;
        measurements->temperature.status = BMS_MEAS_INVALID;

        return BMS_MEAS_DEVICE_ERROR;
    }

    /*
     * Reference protocol:
     *
     *   [0] voltage MSB
     *   [1] voltage LSB
     *   [2] current MSB
     *   [3] current LSB
     *   [4] temperature MSB
     *   [5] temperature LSB
     *
     * All values are big-endian.
     */
    uint16_t raw_voltage =
        ((uint16_t)raw_data[0] << 8) | raw_data[1];

    uint16_t raw_current =
        ((uint16_t)raw_data[2] << 8) | raw_data[3];

    uint16_t raw_temperature =
        ((uint16_t)raw_data[4] << 8) | raw_data[5];

    measurements->voltage.value =
        (float)raw_voltage / BMS_I2C_VOLTAGE_SCALE;

    measurements->current.value =
        (float)raw_current / BMS_I2C_CURRENT_SCALE;

    measurements->temperature.value =
        (float)raw_temperature / BMS_I2C_TEMP_SCALE;

    measurements->voltage.status = BMS_MEAS_VALID;
    measurements->current.status = BMS_MEAS_VALID;
    measurements->temperature.status = BMS_MEAS_VALID;

    return BMS_MEAS_DEVICE_OK;
}
