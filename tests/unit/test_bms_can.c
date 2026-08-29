#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bms_can.h"

static bms_measurements_t make_measurements(float v, float i, float t)
{
    bms_measurements_t m;
    bms_measurements_init(&m);
    m.voltage.value = v;
    m.voltage.status = BMS_MEAS_VALID;
    m.current.value = i;
    m.current.status = BMS_MEAS_VALID;
    m.temperature.value = t;
    m.temperature.status = BMS_MEAS_VALID;
    return m;
}

static bms_state_status_t make_state(bms_state_t s, bms_fault_t f)
{
    bms_state_status_t st;
    st.state = s;
    st.fault = f;
    return st;
}

/* ------------------------------------------------------------
 * Raw byte verification (REQ-034 – encoding)
 * ------------------------------------------------------------ */

static void test_can_raw_bytes_normal(void)
{
    bms_measurements_t m = make_measurements(48.0f, 10.0f, 25.0f);
    bms_state_status_t st = make_state(BMS_STATE_NORMAL, BMS_FAULT_NONE);
    bms_can_frame_t frame;

    bms_can_build_frame(&m, &st, &frame);

    /* Expected raw bytes:
     * Voltage: 48.0 * 100 = 4800 = 0x12C0
     * Current: 10.0 * 1000 = 10000 = 0x2710
     * Temperature: 25.0 * 10 + 1000 = 1250 = 0x04E2
     * State: BMS_STATE_NORMAL = 1
     * Fault: BMS_FAULT_NONE = 0
     */
    assert(frame.data[0] == 0x12);
    assert(frame.data[1] == 0xC0);
    assert(frame.data[2] == 0x27);
    assert(frame.data[3] == 0x10);
    assert(frame.data[4] == 0x04);
    assert(frame.data[5] == 0xE2);
    assert(frame.data[6] == BMS_STATE_NORMAL);
    assert(frame.data[7] == BMS_FAULT_NONE);
}

static void test_can_raw_bytes_negative_current(void)
{
    bms_measurements_t m = make_measurements(48.0f, -10.0f, 25.0f);
    bms_state_status_t st = make_state(BMS_STATE_NORMAL, BMS_FAULT_NONE);
    bms_can_frame_t frame;

    bms_can_build_frame(&m, &st, &frame);

    /* Current: -10.0 * 1000 = -10000 = 0xD8F0 (two's complement) */
    assert(frame.data[2] == 0xD8);
    assert(frame.data[3] == 0xF0);
}

static void test_can_raw_bytes_negative_temperature(void)
{
    bms_measurements_t m = make_measurements(48.0f, 10.0f, -20.0f);
    bms_state_status_t st = make_state(BMS_STATE_FAULT, BMS_FAULT_UNDERTEMPERATURE);
    bms_can_frame_t frame;

    bms_can_build_frame(&m, &st, &frame);

    /* Temperature: -20.0 * 10 + 1000 = 800 = 0x0320 */
    assert(frame.data[4] == 0x03);
    assert(frame.data[5] == 0x20);
    assert(frame.data[6] == BMS_STATE_FAULT);
    assert(frame.data[7] == BMS_FAULT_UNDERTEMPERATURE);
}

/* ------------------------------------------------------------
 * Round-trip decode tests
 * ------------------------------------------------------------ */

static void test_can_build_frame_success_normal(void)
{
    bms_measurements_t m = make_measurements(48.0f, 10.0f, 25.0f);
    bms_state_status_t st = make_state(BMS_STATE_NORMAL, BMS_FAULT_NONE);
    bms_can_frame_t frame;

    int r = bms_can_build_frame(&m, &st, &frame);
    assert(r == 0);
    assert(frame.id == 0x100);
    assert(frame.dlc == 8);

    bms_measurements_t m2;
    bms_state_status_t st2;
    r = bms_can_decode_frame(&frame, &m2, &st2);
    assert(r == 0);

    assert(m2.voltage.value > 47.99f && m2.voltage.value < 48.01f);
    assert(m2.current.value > 9.99f && m2.current.value < 10.01f);
    assert(m2.temperature.value > 24.99f && m2.temperature.value < 25.01f);
    assert(st2.fault == BMS_FAULT_NONE);
    assert(st2.state == BMS_STATE_NORMAL);
}

static void test_can_build_frame_faulty(void)
{
    bms_measurements_t m = make_measurements(55.0f, 10.0f, 25.0f);
    bms_state_status_t st = make_state(BMS_STATE_FAULT, BMS_FAULT_OVERVOLTAGE);
    bms_can_frame_t frame;

    int r = bms_can_build_frame(&m, &st, &frame);
    assert(r == 0);

    bms_measurements_t m2;
    bms_state_status_t st2;
    r = bms_can_decode_frame(&frame, &m2, &st2);
    assert(r == 0);
    assert(m2.voltage.value > 54.99f && m2.voltage.value < 55.01f);
    assert(st2.fault == BMS_FAULT_OVERVOLTAGE);
    assert(st2.state == BMS_STATE_FAULT);
}

static void test_can_build_frame_negative_current(void)
{
    bms_measurements_t m = make_measurements(48.0f, -10.0f, 25.0f);
    bms_state_status_t st = make_state(BMS_STATE_NORMAL, BMS_FAULT_NONE);
    bms_can_frame_t frame;

    int r = bms_can_build_frame(&m, &st, &frame);
    assert(r == 0);

    bms_measurements_t m2;
    bms_state_status_t st2;
    r = bms_can_decode_frame(&frame, &m2, &st2);
    assert(r == 0);
    assert(m2.current.value > -10.01f && m2.current.value < -9.99f);
}

static void test_can_build_frame_negative_temperature(void)
{
    bms_measurements_t m = make_measurements(48.0f, 10.0f, -20.0f);
    bms_state_status_t st = make_state(BMS_STATE_FAULT, BMS_FAULT_UNDERTEMPERATURE);
    bms_can_frame_t frame;

    int r = bms_can_build_frame(&m, &st, &frame);
    assert(r == 0);

    bms_measurements_t m2;
    bms_state_status_t st2;
    r = bms_can_decode_frame(&frame, &m2, &st2);
    assert(r == 0);
    assert(m2.temperature.value > -20.1f && m2.temperature.value < -19.9f);
    assert(st2.fault == BMS_FAULT_UNDERTEMPERATURE);
    assert(st2.state == BMS_STATE_FAULT);
}

static void test_can_build_frame_state_and_fault(void)
{
    bms_measurements_t m = make_measurements(48.0f, 10.0f, 25.0f);

    bms_state_t states[] = {
        BMS_STATE_INIT,
        BMS_STATE_NORMAL,
        BMS_STATE_WARNING,
        BMS_STATE_FAULT
    };
    bms_fault_t faults[] = {
        BMS_FAULT_NONE,
        BMS_FAULT_INVALID_MEASUREMENT,
        BMS_FAULT_INVALID_CONFIGURATION,
        BMS_FAULT_OVERVOLTAGE,
        BMS_FAULT_UNDERVOLTAGE,
        BMS_FAULT_OVERTEMPERATURE,
        BMS_FAULT_UNDERTEMPERATURE,
        BMS_FAULT_OVERCURRENT
    };

    for (size_t si = 0; si < sizeof(states)/sizeof(states[0]); si++)
    {
        for (size_t fi = 0; fi < sizeof(faults)/sizeof(faults[0]); fi++)
        {
            bms_state_status_t st = make_state(states[si], faults[fi]);
            bms_can_frame_t frame;
            bms_state_status_t st2;

            int r = bms_can_build_frame(&m, &st, &frame);
            assert(r == 0);

            bms_measurements_t m2;
            r = bms_can_decode_frame(&frame, &m2, &st2);
            assert(r == 0);

            assert(st2.state == states[si]);
            assert(st2.fault == faults[fi]);
        }
    }
}

static void test_can_build_frame_null_inputs(void)
{
    bms_measurements_t m = make_measurements(48.0f, 10.0f, 25.0f);
    bms_state_status_t st = make_state(BMS_STATE_NORMAL, BMS_FAULT_NONE);
    bms_can_frame_t frame;

    assert(bms_can_build_frame(NULL, &st, &frame) == -1);
    assert(bms_can_build_frame(&m, NULL, &frame) == -1);
    assert(bms_can_build_frame(&m, &st, NULL) == -1);
}

static void test_can_decode_frame_null_inputs(void)
{
    bms_can_frame_t frame = { .id = 0x100, .dlc = 8, .data = {0} };
    bms_measurements_t m;
    bms_state_status_t st;

    assert(bms_can_decode_frame(NULL, &m, &st) == -1);
    assert(bms_can_decode_frame(&frame, NULL, &st) == -1);
    assert(bms_can_decode_frame(&frame, &m, NULL) == -1);
}

static void test_can_frame_is_valid(void)
{
    bms_measurements_t m = make_measurements(48.0f, 10.0f, 25.0f);
    bms_state_status_t st = make_state(BMS_STATE_NORMAL, BMS_FAULT_NONE);
    bms_can_frame_t frame;

    bms_can_build_frame(&m, &st, &frame);
    assert(bms_can_frame_is_valid(&frame) == 1);

    frame.dlc = 7;
    assert(bms_can_frame_is_valid(&frame) == 0);
    frame.dlc = 8;

    frame.id = 0x200;
    assert(bms_can_frame_is_valid(&frame) == 0);
    frame.id = 0x100;

    assert(bms_can_frame_is_valid(NULL) == 0);
}

static void test_can_set_base_id(void)
{
    bms_measurements_t m = make_measurements(48.0f, 10.0f, 25.0f);
    bms_state_status_t st = make_state(BMS_STATE_NORMAL, BMS_FAULT_NONE);
    bms_can_frame_t frame;

    assert(bms_can_set_base_id(0x200) == 0);
    bms_can_build_frame(&m, &st, &frame);
    assert(frame.id == 0x200);
    assert(bms_can_frame_is_valid(&frame) == 1);

    assert(bms_can_set_base_id(0) == -1); /* invalid by policy */
    assert(bms_can_set_base_id(0x20000000) == -1); /* out of 29-bit range */

    bms_can_set_base_id(0x100); /* reset for other tests */
}

static void test_can_build_frame_rounding_negative(void)
{
    bms_measurements_t m = make_measurements(48.0f, -10.0f, -20.0f);
    bms_state_status_t st = make_state(BMS_STATE_FAULT, BMS_FAULT_UNDERTEMPERATURE);
    bms_can_frame_t frame;

    int r = bms_can_build_frame(&m, &st, &frame);
    assert(r == 0);

    bms_measurements_t m2;
    bms_state_status_t st2;
    r = bms_can_decode_frame(&frame, &m2, &st2);
    assert(r == 0);

    assert(m2.current.value > -10.01f && m2.current.value < -9.99f);
    assert(m2.temperature.value > -20.1f && m2.temperature.value < -19.9f);
}

/* ------------------------------------------------------------
 * HARDENED CLAMP TESTS
 * ------------------------------------------------------------ */

static void test_can_clamp_values(void)
{
    /* ---- Voltage clamp (negative -> 0, >655.35 -> 655.35) ---- */
    bms_measurements_t m = make_measurements(-10.0f, 0.0f, 0.0f);
    bms_state_status_t st = make_state(BMS_STATE_NORMAL, BMS_FAULT_NONE);
    bms_can_frame_t frame;

    bms_can_build_frame(&m, &st, &frame);
    bms_measurements_t m2;
    bms_state_status_t st2;
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.voltage.value == 0.0f); /* clamped to 0 */

    m = make_measurements(1000.0f, 0.0f, 0.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.voltage.value > 655.34f && m2.voltage.value < 655.36f); /* clamped to max */

    /* ---- Current clamp (negative below -32.767, positive above +32.767) ---- */
    m = make_measurements(0.0f, -100.0f, 0.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.current.value > -32.77f && m2.current.value < -32.76f); /* clamped to -32.767 */

    m = make_measurements(0.0f, 100.0f, 0.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.current.value > 32.76f && m2.current.value < 32.78f); /* clamped to +32.767 */

    /* ---- Temperature clamp (upper bound and overflow) ---- */
    /* 1000 °C should encode as 1000 °C (no clamp) */
    m = make_measurements(0.0f, 0.0f, 1000.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.temperature.value > 999.9f && m2.temperature.value < 1000.1f);

    /* 6453.5 °C is the exact upper bound ((65535 - 1000) / 10) */
    m = make_measurements(0.0f, 0.0f, 6453.5f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.temperature.value > 6453.4f && m2.temperature.value < 6453.6f);

    /* 10000 °C should clamp to 6453.5 °C */
    m = make_measurements(0.0f, 0.0f, 10000.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.temperature.value > 6453.4f && m2.temperature.value < 6453.6f);

    /* ---- Temperature lower bound: should allow negative temperatures down to -100 °C ---- */
    /* -100 °C -> (-100 * 10 + 1000) = 0, should encode as -100 °C */
    m = make_measurements(0.0f, 0.0f, -100.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.temperature.value > -100.1f && m2.temperature.value < -99.9f);

    /* -1000 °C should clamp to -100 °C (since offset makes it negative => clamp to 0 -> -100 °C) */
    m = make_measurements(0.0f, 0.0f, -1000.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.temperature.value > -100.1f && m2.temperature.value < -99.9f);
}

/* ------------------------------------------------------------
 * Invalid frame tests
 * ------------------------------------------------------------ */

static void test_can_frame_invalid_dlc(void)
{
    bms_can_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.id = 0x100;
    frame.dlc = 7; /* invalid */
    assert(bms_can_frame_is_valid(&frame) == 0);
}

static void test_can_frame_invalid_id(void)
{
    bms_can_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.id = 0x200;
    frame.dlc = 8;
    assert(bms_can_frame_is_valid(&frame) == 0);
}

static void test_can_frame_invalid_state_and_fault(void)
{
    bms_can_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    frame.id = 0x100;
    frame.dlc = 8;

    /* Invalid enum values on the CAN wire */
    frame.data[6] = 0xFF;
    frame.data[7] = 0xFF;

    assert(bms_can_frame_is_valid(&frame) == 0);
}

int main(void)
{
    printf("[BMS CAN UNIT] Running tests...\n");

    /* Raw byte tests (REQ-034) */
    test_can_raw_bytes_normal();
    printf("[PASS] raw_bytes_normal\n");

    test_can_raw_bytes_negative_current();
    printf("[PASS] raw_bytes_negative_current\n");

    test_can_raw_bytes_negative_temperature();
    printf("[PASS] raw_bytes_negative_temperature\n");

    /* Round-trip decode tests */
    test_can_build_frame_success_normal();
    printf("[PASS] build_frame_success_normal\n");

    test_can_build_frame_faulty();
    printf("[PASS] build_frame_faulty\n");

    test_can_build_frame_negative_current();
    printf("[PASS] build_frame_negative_current\n");

    test_can_build_frame_negative_temperature();
    printf("[PASS] build_frame_negative_temperature\n");

    test_can_build_frame_state_and_fault();
    printf("[PASS] build_frame_state_and_fault\n");

    test_can_build_frame_null_inputs();
    printf("[PASS] build_frame_null_inputs\n");

    test_can_decode_frame_null_inputs();
    printf("[PASS] decode_frame_null_inputs\n");

    test_can_frame_is_valid();
    printf("[PASS] frame_is_valid\n");

    test_can_set_base_id();
    printf("[PASS] set_base_id\n");

    test_can_build_frame_rounding_negative();
    printf("[PASS] build_frame_rounding_negative\n");

    /* New clamp tests */
    test_can_clamp_values();
    printf("[PASS] clamp_values\n");

    test_can_frame_invalid_dlc();
    printf("[PASS] invalid_dlc\n");

    test_can_frame_invalid_id();
    printf("[PASS] invalid_id\n");

    test_can_frame_invalid_state_and_fault();
    printf("[PASS] invalid_state_and_fault\n");

    printf("[BMS CAN UNIT] All tests passed.\n");
    return 0;
}
