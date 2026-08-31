#include <stdio.h>

#include "bms_measurements.h"
#include "bms_measurement_device.h"

/*
 * BMS I2C / Measurement Abstraction Tests
 *
 * Requirements under verification:
 *
 *   BMS-REQ-038 — Measurement Device Abstraction
 *   BMS-REQ-039 — I2C Error Propagation
 *   BMS-REQ-040 — Measurement Communication Failure
 *
 * IMPORTANT:
 * This file intentionally tests the abstract measurement-device
 * interface and does not depend directly on src/drivers/i2c.c.
 *
 * The production interface is expected to provide:
 *
 *   bms_measurement_device_t
 *   bms_measurement_result_t
 *   bms_measurement_device_read()
 */

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_TRUE(condition)                                      \
    do                                                              \
    {                                                               \
        tests_run++;                                                \
        if (!(condition))                                           \
        {                                                           \
            fprintf(stderr,                                          \
                    "[FAIL] %s:%d: %s\n",                           \
                    __FILE__,                                       \
                    __LINE__,                                       \
                    #condition);                                    \
            return 1;                                               \
        }                                                           \
        tests_passed++;                                             \
    } while (0)

#define ASSERT_EQ(expected, actual)                                 \
    do                                                              \
    {                                                               \
        tests_run++;                                                \
        if ((expected) != (actual))                                 \
        {                                                           \
            fprintf(stderr,                                          \
                    "[FAIL] %s:%d: expected %d, got %d\n",          \
                    __FILE__,                                       \
                    __LINE__,                                       \
                    (expected),                                     \
                    (actual));                                      \
            return 1;                                               \
        }                                                           \
        tests_passed++;                                             \
    } while (0)


/* ================================================================
 * Fake measurement-device callbacks
 *
 * These callbacks represent the device/transport boundary.
 * No real I2C hardware is accessed by this test.
 * ================================================================ */

static bms_measurement_result_t fake_read_success(
    void *context,
    bms_measurement_t *measurement)
{
    (void)context;

    if (measurement == NULL)
    {
        return BMS_MEASUREMENT_ERR_INVALID;
    }

    measurement->value = 48.0f;
    measurement->status = BMS_MEAS_VALID;

    return BMS_MEASUREMENT_OK;
}

static bms_measurement_result_t fake_read_nack(
    void *context,
    bms_measurement_t *measurement)
{
    (void)context;
    (void)measurement;

    return BMS_MEASUREMENT_ERR_NACK;
}

static bms_measurement_result_t fake_read_timeout(
    void *context,
    bms_measurement_t *measurement)
{
    (void)context;
    (void)measurement;

    return BMS_MEASUREMENT_ERR_TIMEOUT;
}

static bms_measurement_result_t fake_read_bus_error(
    void *context,
    bms_measurement_t *measurement)
{
    (void)context;
    (void)measurement;

    return BMS_MEASUREMENT_ERR_BUS;
}

static bms_measurement_result_t fake_read_arbitration_error(
    void *context,
    bms_measurement_t *measurement)
{
    (void)context;
    (void)measurement;

    return BMS_MEASUREMENT_ERR_ARBITRATION;
}


/* ================================================================
 * REQ-038 — Measurement Device Abstraction
 * ================================================================ */

static int test_req038_measurement_obtained_through_abstraction(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_success
    };

    bms_measurement_t measurement = {
        .value = 0.0f,
        .status = BMS_MEAS_NOT_AVAILABLE
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_OK, result);
    ASSERT_TRUE(measurement.value == 48.0f);
    ASSERT_EQ(BMS_MEAS_VALID, measurement.status);

    return 0;
}


/* ================================================================
 * REQ-039 — I2C Error Propagation
 * ================================================================ */

static int test_req039_nack_is_propagated(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_nack
    };

    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_VALID
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_NACK, result);

    return 0;
}

static int test_req039_timeout_is_propagated(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_timeout
    };

    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_VALID
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_TIMEOUT, result);

    return 0;
}

static int test_req039_bus_error_is_propagated(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_bus_error
    };

    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_VALID
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_BUS, result);

    return 0;
}

static int test_req039_arbitration_error_is_propagated(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_arbitration_error
    };

    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_VALID
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_ARBITRATION, result);

    return 0;
}


/* ================================================================
 * REQ-040 — Measurement Communication Failure
 * ================================================================ */

static int test_req040_nack_does_not_produce_valid_measurement(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_nack
    };

    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_VALID
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_NACK, result);
    ASSERT_EQ(BMS_MEAS_INVALID, measurement.status);

    return 0;
}

static int test_req040_timeout_does_not_produce_valid_measurement(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_timeout
    };

    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_VALID
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_TIMEOUT, result);
    ASSERT_EQ(BMS_MEAS_INVALID, measurement.status);

    return 0;
}

static int test_req040_bus_error_does_not_produce_valid_measurement(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_bus_error
    };

    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_VALID
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_BUS, result);
    ASSERT_EQ(BMS_MEAS_INVALID, measurement.status);

    return 0;
}

static int test_req040_arbitration_error_does_not_produce_valid_measurement(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_arbitration_error
    };

    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_VALID
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_ARBITRATION, result);
    ASSERT_EQ(BMS_MEAS_INVALID, measurement.status);

    return 0;
}


/* ================================================================
 * Additional interface robustness tests
 * ================================================================ */

static int test_null_device_is_rejected(void)
{
    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_VALID
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(NULL, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_INVALID, result);
    ASSERT_EQ(BMS_MEAS_INVALID, measurement.status);

    return 0;
}

static int test_null_measurement_is_rejected(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_success
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, NULL);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_INVALID, result);

    return 0;
}


/* ================================================================
 * NEW: Success-path contract test (REQ-038 / REQ-040)
 *
 * This test enforces that even if the callback returns
 * BMS_MEASUREMENT_OK, the abstraction must still reject the
 * measurement unless the callback also set BMS_MEAS_VALID.
 * ================================================================ */

static bms_measurement_result_t fake_read_success_without_valid_status(
    void *context,
    bms_measurement_t *measurement)
{
    (void)context;
    (void)measurement;

    return BMS_MEASUREMENT_OK;
}

static int test_success_requires_valid_measurement(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = fake_read_success_without_valid_status
    };

    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_NOT_AVAILABLE
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    /* The abstraction must reject this as invalid */
    ASSERT_EQ(BMS_MEASUREMENT_ERR_INVALID, result);
    ASSERT_EQ(BMS_MEAS_INVALID, measurement.status);

    return 0;
}


/* ================================================================
 * NEW: Null callback rejection test
 *
 * This test enforces that the abstraction rejects a device with
 * a NULL read callback.
 * ================================================================ */

static int test_null_read_callback_is_rejected(void)
{
    bms_measurement_device_t device = {
        .context = NULL,
        .read = NULL
    };

    bms_measurement_t measurement = {
        .value = 48.0f,
        .status = BMS_MEAS_VALID
    };

    bms_measurement_result_t result;

    result = bms_measurement_device_read(&device, &measurement);

    ASSERT_EQ(BMS_MEASUREMENT_ERR_INVALID, result);
    ASSERT_EQ(BMS_MEAS_INVALID, measurement.status);

    return 0;
}


/* ================================================================
 * Test runner
 * ================================================================ */

int main(void)
{
    printf("[BMS I2C ABSTRACTION UNIT] Running tests...\n");

    /* REQ-038 */
    if (test_req038_measurement_obtained_through_abstraction() != 0)
        return 1;

    /* REQ-039 */
    if (test_req039_nack_is_propagated() != 0)
        return 1;

    if (test_req039_timeout_is_propagated() != 0)
        return 1;

    if (test_req039_bus_error_is_propagated() != 0)
        return 1;

    if (test_req039_arbitration_error_is_propagated() != 0)
        return 1;

    /* REQ-040 */
    if (test_req040_nack_does_not_produce_valid_measurement() != 0)
        return 1;

    if (test_req040_timeout_does_not_produce_valid_measurement() != 0)
        return 1;

    if (test_req040_bus_error_does_not_produce_valid_measurement() != 0)
        return 1;

    if (test_req040_arbitration_error_does_not_produce_valid_measurement() != 0)
        return 1;

    /* Interface robustness */
    if (test_null_device_is_rejected() != 0)
        return 1;

    if (test_null_measurement_is_rejected() != 0)
        return 1;

    /* Success-path contract test */
    if (test_success_requires_valid_measurement() != 0)
        return 1;

    /* Null callback rejection test */
    if (test_null_read_callback_is_rejected() != 0)
        return 1;

    printf("[BMS I2C ABSTRACTION UNIT] %d/%d tests passed.\n",
           tests_passed,
           tests_run);

    return 0;
}