#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bms_i2c_measurement_device.h"
#include "drivers/i2c.h"

/* Mock state. */
static int mock_i2c_status = I2C_OK;
static uint8_t mock_i2c_data[6];
static uint8_t mock_last_dev_addr;
static uint8_t mock_last_start_reg;
static unsigned int mock_last_length;

/*
 * Host-test replacement for the hardware I2C transaction.
 */
int i2c_read_burst(
    unsigned char dev_addr,
    unsigned char reg,
    unsigned char *buf,
    unsigned int len)
{
    mock_last_dev_addr = dev_addr;
    mock_last_start_reg = reg;
    mock_last_length = len;

    if (mock_i2c_status != I2C_OK)
    {
        return mock_i2c_status;
    }

    if (buf == 0 || len < 6U)
    {
        return I2C_ERR_BUS;
    }

    memcpy(buf, mock_i2c_data, 6U);

    return I2C_OK;
}

static void reset_mock(void)
{
    mock_i2c_status = I2C_OK;

    mock_last_dev_addr = 0U;
    mock_last_start_reg = 0U;
    mock_last_length = 0U;

    /* 12.345 V -> 12345 -> 0x3039 */
    mock_i2c_data[0] = 0x30;
    mock_i2c_data[1] = 0x39;

    /* 0.500 A -> 500 -> 0x01F4 */
    mock_i2c_data[2] = 0x01;
    mock_i2c_data[3] = 0xF4;

    /* 40.0 C -> 400 -> 0x0190 */
    mock_i2c_data[4] = 0x01;
    mock_i2c_data[5] = 0x90;
}

static void assert_invalid_measurements(
    const bms_measurements_t *measurements)
{
    assert(measurements->voltage.status == BMS_MEAS_INVALID);
    assert(measurements->current.status == BMS_MEAS_INVALID);
    assert(measurements->temperature.status == BMS_MEAS_INVALID);

    assert(measurements->voltage.value == 0.0f);
    assert(measurements->current.value == 0.0f);
    assert(measurements->temperature.value == 0.0f);
}

static void test_successful_read(void)
{
    reset_mock();

    bms_i2c_measurement_context_t ctx = {
        .dev_addr = 0x40,
        .timeout_ms = 10U
    };

    bms_measurements_t measurements;

    bms_measurement_device_status_t result =
        bms_i2c_measurement_device_read(&measurements, &ctx);

    assert(result == BMS_MEAS_DEVICE_OK);

    assert(measurements.voltage.status == BMS_MEAS_VALID);
    assert(measurements.current.status == BMS_MEAS_VALID);
    assert(measurements.temperature.status == BMS_MEAS_VALID);

    assert(measurements.voltage.value > 12.344f);
    assert(measurements.voltage.value < 12.346f);

    assert(measurements.current.value > 0.499f);
    assert(measurements.current.value < 0.501f);

    assert(measurements.temperature.value > 39.99f);
    assert(measurements.temperature.value < 40.01f);

    /* Verify adapter used the specified reference transaction. */
    assert(mock_last_dev_addr == 0x40U);
    assert(mock_last_start_reg == 0x00U);
    assert(mock_last_length == 6U);
}

static void test_nack_error(void)
{
    reset_mock();
    mock_i2c_status = I2C_ERR_NACK;

    bms_i2c_measurement_context_t ctx = {
        .dev_addr = 0x40
    };

    bms_measurements_t measurements;

    bms_measurement_device_status_t result =
        bms_i2c_measurement_device_read(&measurements, &ctx);

    assert(result == BMS_MEAS_DEVICE_ERROR);
    assert_invalid_measurements(&measurements);
}

static void test_timeout_error(void)
{
    reset_mock();
    mock_i2c_status = I2C_ERR_TOUT;

    bms_i2c_measurement_context_t ctx = {
        .dev_addr = 0x40
    };

    bms_measurements_t measurements;

    bms_measurement_device_status_t result =
        bms_i2c_measurement_device_read(&measurements, &ctx);

    assert(result == BMS_MEAS_DEVICE_ERROR);
    assert_invalid_measurements(&measurements);
}

static void test_bus_error(void)
{
    reset_mock();
    mock_i2c_status = I2C_ERR_BUS;

    bms_i2c_measurement_context_t ctx = {
        .dev_addr = 0x40
    };

    bms_measurements_t measurements;

    bms_measurement_device_status_t result =
        bms_i2c_measurement_device_read(&measurements, &ctx);

    assert(result == BMS_MEAS_DEVICE_ERROR);
    assert_invalid_measurements(&measurements);
}

static void test_arbitration_error(void)
{
    reset_mock();
    mock_i2c_status = I2C_ERR_ARB;

    bms_i2c_measurement_context_t ctx = {
        .dev_addr = 0x40
    };

    bms_measurements_t measurements;

    bms_measurement_device_status_t result =
        bms_i2c_measurement_device_read(&measurements, &ctx);

    assert(result == BMS_MEAS_DEVICE_ERROR);
    assert_invalid_measurements(&measurements);
}

static void test_null_context(void)
{
    reset_mock();

    bms_measurements_t measurements;

    bms_measurement_device_status_t result =
        bms_i2c_measurement_device_read(&measurements, 0);

    assert(result == BMS_MEAS_DEVICE_ERROR);
}

static void test_null_measurements(void)
{
    reset_mock();

    bms_i2c_measurement_context_t ctx = {
        .dev_addr = 0x40
    };

    bms_measurement_device_status_t result =
        bms_i2c_measurement_device_read(0, &ctx);

    assert(result == BMS_MEAS_DEVICE_ERROR);
}

int main(void)
{
    printf("[BMS I2C MEASUREMENT DEVICE UNIT] Running tests...\n");

    test_successful_read();
    printf("[PASS] successful_read\n");

    test_nack_error();
    printf("[PASS] nack_error\n");

    test_timeout_error();
    printf("[PASS] timeout_error\n");

    test_bus_error();
    printf("[PASS] bus_error\n");

    test_arbitration_error();
    printf("[PASS] arbitration_error\n");

    test_null_context();
    printf("[PASS] null_context\n");

    test_null_measurements();
    printf("[PASS] null_measurements\n");

    printf("[BMS I2C MEASUREMENT DEVICE UNIT] All tests passed.\n");

    return 0;
}
