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

/* ΝΕΕΣ ΔΟΚΙΜΕΣ ΜΕΤΑΒΑΣΕΩΝ */
static void test_transition_normal_to_fault_to_normal(void)
{
    bms_state_status_t status;
    bms_state_init(&status);

    /* 1. Αρχικά NORMAL */
    bms_state_update(&status, BMS_PROTECTION_NORMAL);
    assert(status.state == BMS_STATE_NORMAL);
    assert(status.fault == BMS_FAULT_NONE);

    /* 2. Πηγαίνουμε σε OVERVOLTAGE → FAULT */
    bms_state_update(&status, BMS_PROTECTION_OVER_VOLTAGE);
    assert(status.state == BMS_STATE_FAULT);
    assert(status.fault == BMS_FAULT_OVERVOLTAGE);

    /* 3. Επιστροφή σε NORMAL */
    bms_state_update(&status, BMS_PROTECTION_NORMAL);
    assert(status.state == BMS_STATE_NORMAL);
    assert(status.fault == BMS_FAULT_NONE);
}

static void test_transition_fault_to_normal(void)
{
    bms_state_status_t status;
    bms_state_init(&status);

    /* Ξεκινάμε από FAULT (π.χ. OVERTEMPERATURE) */
    bms_state_update(&status, BMS_PROTECTION_OVER_TEMPERATURE);
    assert(status.state == BMS_STATE_FAULT);
    assert(status.fault == BMS_FAULT_OVERTEMPERATURE);

    /* Επιστροφή σε NORMAL */
    bms_state_update(&status, BMS_PROTECTION_NORMAL);
    assert(status.state == BMS_STATE_NORMAL);
    assert(status.fault == BMS_FAULT_NONE);
}

static void test_transition_multiple_faults(void)
{
    bms_state_status_t status;
    bms_state_init(&status);

    /* NORMAL → OVERVOLTAGE → UNDERVOLTAGE → NORMAL */
    bms_state_update(&status, BMS_PROTECTION_NORMAL);
    assert(status.state == BMS_STATE_NORMAL);

    bms_state_update(&status, BMS_PROTECTION_OVER_VOLTAGE);
    assert(status.state == BMS_STATE_FAULT);
    assert(status.fault == BMS_FAULT_OVERVOLTAGE);

    bms_state_update(&status, BMS_PROTECTION_UNDER_VOLTAGE);
    assert(status.state == BMS_STATE_FAULT);
    assert(status.fault == BMS_FAULT_UNDERVOLTAGE);

    bms_state_update(&status, BMS_PROTECTION_NORMAL);
    assert(status.state == BMS_STATE_NORMAL);
    assert(status.fault == BMS_FAULT_NONE);
}

static void test_null_init(void) { bms_state_init(0); }
static void test_null_update(void) { bms_state_update(0, BMS_PROTECTION_NORMAL); }

int main(void)
{
    printf("[BMS STATE UNIT] Running tests with transitions...\n");

    test_init();
    printf("[PASS] init\n");

    test_normal();
    printf("[PASS] normal\n");

    test_invalid_measurement();
    printf("[PASS] invalid_measurement\n");

    test_transition_normal_to_fault_to_normal();
    printf("[PASS] transition normal → fault → normal\n");

    test_transition_fault_to_normal();
    printf("[PASS] transition fault → normal\n");

    test_transition_multiple_faults();
    printf("[PASS] multiple faults transitions\n");

    test_null_init();
    printf("[PASS] null_init\n");

    test_null_update();
    printf("[PASS] null_update\n");

    printf("[BMS STATE UNIT] All tests passed.\n");
    return 0;
}
