#include <assert.h>
#include <stdio.h>

#include "bms_state.h"

static void test_init(void)
{
    bms_state_status_t status;

    bms_state_init(&status);

    assert(status.state == BMS_STATE_INIT);
    assert(status.fault == BMS_FAULT_NONE);
}

static void test_normal(void)
{
    bms_state_status_t status;

    bms_state_init(&status);
    bms_state_update(&status, BMS_PROTECTION_NORMAL);

    assert(status.state == BMS_STATE_NORMAL);
    assert(status.fault == BMS_FAULT_NONE);
}

static void test_invalid_measurement(void)
{
    bms_state_status_t status;

    bms_state_init(&status);
    bms_state_update(&status, BMS_PROTECTION_INVALID_MEASUREMENT);

    assert(status.state == BMS_STATE_FAULT);
    assert(status.fault == BMS_FAULT_INVALID_MEASUREMENT);
}

static void test_overvoltage(void)
{
    bms_state_status_t status;

    bms_state_init(&status);
    bms_state_update(&status, BMS_PROTECTION_OVER_VOLTAGE);

    assert(status.state == BMS_STATE_FAULT);
    assert(status.fault == BMS_FAULT_OVERVOLTAGE);
}

static void test_undervoltage(void)
{
    bms_state_status_t status;

    bms_state_init(&status);
    bms_state_update(&status, BMS_PROTECTION_UNDER_VOLTAGE);

    assert(status.state == BMS_STATE_FAULT);
    assert(status.fault == BMS_FAULT_UNDERVOLTAGE);
}

static void test_overcurrent(void)
{
    bms_state_status_t status;

    bms_state_init(&status);
    bms_state_update(&status, BMS_PROTECTION_OVER_CURRENT);

    assert(status.state == BMS_STATE_FAULT);
    assert(status.fault == BMS_FAULT_OVERCURRENT);
}

static void test_overtemperature(void)
{
    bms_state_status_t status;

    bms_state_init(&status);
    bms_state_update(&status, BMS_PROTECTION_OVER_TEMPERATURE);

    assert(status.state == BMS_STATE_FAULT);
    assert(status.fault == BMS_FAULT_OVERTEMPERATURE);
}

static void test_undertemperature(void)
{
    bms_state_status_t status;

    bms_state_init(&status);
    bms_state_update(&status, BMS_PROTECTION_UNDER_TEMPERATURE);

    assert(status.state == BMS_STATE_FAULT);
    assert(status.fault == BMS_FAULT_UNDERTEMPERATURE);
}

static void test_null_init(void)
{
    bms_state_init(0);
}

static void test_null_update(void)
{
    bms_state_update(0, BMS_PROTECTION_NORMAL);
}

int main(void)
{
    printf("[BMS STATE UNIT] Running 10 tests...\n");

    test_init();
    printf("[PASS] init\n");

    test_normal();
    printf("[PASS] normal\n");

    test_invalid_measurement();
    printf("[PASS] invalid_measurement\n");

    test_overvoltage();
    printf("[PASS] overvoltage\n");

    test_undervoltage();
    printf("[PASS] undervoltage\n");

    test_overcurrent();
    printf("[PASS] overcurrent\n");

    test_overtemperature();
    printf("[PASS] overtemperature\n");

    test_undertemperature();
    printf("[PASS] undertemperature\n");

    test_null_init();
    printf("[PASS] null_init\n");

    test_null_update();
    printf("[PASS] null_update\n");

    printf("[BMS STATE UNIT] All tests passed.\n");

    return 0;
}