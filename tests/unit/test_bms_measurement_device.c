#include <assert.h>
#include <stdio.h>

#include "bms_measurement_device.h"

static bms_measurement_device_status_t
mock_read_success(bms_measurements_t *measurements, void *context)
{
    (void)context;

    measurements->voltage.value = 48.0f;
    measurements->voltage.status = BMS_MEAS_VALID;

    measurements->current.value = 10.0f;
    measurements->current.status = BMS_MEAS_VALID;

    measurements->temperature.value = 25.0f;
    measurements->temperature.status = BMS_MEAS_VALID;

    return BMS_MEAS_DEVICE_OK;
}

static bms_measurement_device_status_t
mock_read_failure(bms_measurements_t *measurements, void *context)
{
    (void)measurements;
    (void)context;

    return BMS_MEAS_DEVICE_ERROR;
}

static void test_device_initialization(void)
{
    bms_measurement_device_t device;

    bms_measurement_device_init(
        &device,
        mock_read_success,
        NULL
    );

    assert(device.read == mock_read_success);
    assert(device.context == NULL);
}

/* BMS-REQ-001 — Battery Voltage Measurement
 * BMS-REQ-002 — Battery Current Measurement
 * BMS-REQ-003 — Battery Temperature Measurement
 */
static void test_successful_measurement_read(void)
{
    bms_measurement_device_t device;
    bms_measurements_t measurements;

    bms_measurement_device_init(
        &device,
        mock_read_success,
        NULL
    );

    bms_measurements_init(&measurements);

    assert(
        bms_measurement_device_read(
            &device,
            &measurements
        ) == BMS_MEAS_DEVICE_OK
    );

    assert(measurements.voltage.value == 48.0f);
    assert(measurements.current.value == 10.0f);
    assert(measurements.temperature.value == 25.0f);

    assert(measurements.voltage.status == BMS_MEAS_VALID);
    assert(measurements.current.status == BMS_MEAS_VALID);
    assert(measurements.temperature.status == BMS_MEAS_VALID);
}

/* BMS-REQ-039 / BMS-REQ-040 — measurement communication failure
 * Generic acquisition failure propagation is verified here.
 */
static void test_communication_error_propagation(void)
{
    bms_measurement_device_t device;
    bms_measurements_t measurements;

    bms_measurement_device_init(
        &device,
        mock_read_failure,
        NULL
    );

    bms_measurements_init(&measurements);

    assert(
        bms_measurement_device_read(
            &device,
            &measurements
        ) == BMS_MEAS_DEVICE_ERROR
    );

    assert(measurements.voltage.status != BMS_MEAS_VALID);
    assert(measurements.current.status != BMS_MEAS_VALID);
    assert(measurements.temperature.status != BMS_MEAS_VALID);
}

static void test_invalid_device_arguments(void)
{
    bms_measurements_t measurements;

    bms_measurements_init(&measurements);

    assert(
        bms_measurement_device_read(
            NULL,
            &measurements
        ) == BMS_MEAS_DEVICE_ERROR
    );

    assert(
        bms_measurement_device_read(
            NULL,
            NULL
        ) == BMS_MEAS_DEVICE_ERROR
    );

    {
        bms_measurement_device_t device;

        bms_measurement_device_init(
            &device,
            NULL,
            NULL
        );

        assert(
            bms_measurement_device_read(
                &device,
                &measurements
            ) == BMS_MEAS_DEVICE_ERROR
        );
    }
}

int main(void)
{
    test_device_initialization();
    test_successful_measurement_read();
    test_communication_error_propagation();
    test_invalid_device_arguments();

    printf("BMS measurement device tests passed.\n");

    return 0;
}
