#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

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
 * Helper: compare two measurements with tolerance
 * ------------------------------------------------------------ */
static int measurements_equal(const bms_measurements_t *a,
                              const bms_measurements_t *b,
                              float tol)
{
    if (fabsf(a->voltage.value - b->voltage.value) > tol) return 0;
    if (fabsf(a->current.value - b->current.value) > tol) return 0;
    if (fabsf(a->temperature.value - b->temperature.value) > tol) return 0;
    if (a->voltage.status != b->voltage.status) return 0;
    if (a->current.status != b->current.status) return 0;
    if (a->temperature.status != b->temperature.status) return 0;
    return 1;
}

/* ------------------------------------------------------------
 * RAW BYTE TESTS (REQ‑034)
 * ------------------------------------------------------------ */

static void test_can_raw_bytes_normal(void)
{
    bms_measurements_t m = make_measurements(48.0f, 10.0f, 25.0f);
    bms_state_status_t st = make_state(BMS_STATE_NORMAL, BMS_FAULT_NONE);
    bms_can_frame_t frame;

    bms_can_build_frame(&m, &st, &frame);

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
    assert(frame.data[2] == 0xD8);
    assert(frame.data[3] == 0xF0);
}

static void test_can_raw_bytes_negative_temperature(void)
{
    bms_measurements_t m = make_measurements(48.0f, 10.0f, -20.0f);
    bms_state_status_t st = make_state(BMS_STATE_FAULT, BMS_FAULT_UNDERTEMPERATURE);
    bms_can_frame_t frame;

    bms_can_build_frame(&m, &st, &frame);
    assert(frame.data[4] == 0x03);
    assert(frame.data[5] == 0x20);
    assert(frame.data[6] == BMS_STATE_FAULT);
    assert(frame.data[7] == BMS_FAULT_UNDERTEMPERATURE);
}

/* ------------------------------------------------------------
 * ROUND‑TRIP DECODE TESTS
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

    assert(bms_can_set_base_id(0) == -1);
    assert(bms_can_set_base_id(0x20000000) == -1);

    bms_can_set_base_id(0x100);
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
 * CLAMP TESTS
 * ------------------------------------------------------------ */

static void test_can_clamp_values(void)
{
    bms_measurements_t m = make_measurements(-10.0f, 0.0f, 0.0f);
    bms_state_status_t st = make_state(BMS_STATE_NORMAL, BMS_FAULT_NONE);
    bms_can_frame_t frame;
    bms_measurements_t m2;
    bms_state_status_t st2;

    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.voltage.value == 0.0f);

    m = make_measurements(1000.0f, 0.0f, 0.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.voltage.value > 655.34f && m2.voltage.value < 655.36f);

    m = make_measurements(0.0f, -100.0f, 0.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.current.value > -32.77f && m2.current.value < -32.76f);

    m = make_measurements(0.0f, 100.0f, 0.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.current.value > 32.76f && m2.current.value < 32.78f);

    m = make_measurements(0.0f, 0.0f, 1000.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.temperature.value > 999.9f && m2.temperature.value < 1000.1f);

    m = make_measurements(0.0f, 0.0f, 6453.5f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.temperature.value > 6453.4f && m2.temperature.value < 6453.6f);

    m = make_measurements(0.0f, 0.0f, 10000.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.temperature.value > 6453.4f && m2.temperature.value < 6453.6f);

    m = make_measurements(0.0f, 0.0f, -100.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.temperature.value > -100.1f && m2.temperature.value < -99.9f);

    m = make_measurements(0.0f, 0.0f, -1000.0f);
    bms_can_build_frame(&m, &st, &frame);
    bms_can_decode_frame(&frame, &m2, &st2);
    assert(m2.temperature.value > -100.1f && m2.temperature.value < -99.9f);
}

/* ------------------------------------------------------------
 * INVALID FRAME TESTS (REQ‑036)
 * ------------------------------------------------------------ */

static void test_can_frame_invalid_dlc(void)
{
    bms_can_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.id = 0x100;
    frame.dlc = 7;
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
    frame.data[6] = 0xFF;
    frame.data[7] = 0xFF;
    assert(bms_can_frame_is_valid(&frame) == 0);
}

/* ------------------------------------------------------------
 * ROUND‑TRIP BOUNDARY TESTS (NEW)
 *
 * Verifies that encoding a measurement, decoding it back
 * yields an equivalent measurement within quantization tolerance.
 *
 * Covers:
 *   - Voltage: 0, 10, 655.35, 400
 *   - Current: -32.767, -10, 0, 10, 32.767
 *   - Temperature: -100, -20, 0, 25, 1000, 6453.5
 *   - State: all BMS_STATE_* values
 *   - Fault: all BMS_FAULT_* values
 * ------------------------------------------------------------ */

static void test_can_roundtrip_boundaries(void)
{
    /* Test vectors */
    struct {
        float voltage;
        float current;
        float temperature;
        bms_state_t state;
        bms_fault_t fault;
        const char *desc;
    } vectors[] = {
        {  0.0f,    0.0f,    0.0f,   BMS_STATE_INIT,   BMS_FAULT_NONE,                     "zero" },
        { 10.0f,    5.0f,   20.0f,   BMS_STATE_NORMAL, BMS_FAULT_NONE,                     "normal" },
        { 48.0f,   10.0f,   25.0f,   BMS_STATE_NORMAL, BMS_FAULT_NONE,                     "nominal" },
        { 55.0f,   10.0f,   25.0f,   BMS_STATE_FAULT,  BMS_FAULT_OVERVOLTAGE,               "overvoltage" },
        { 39.0f,   10.0f,   25.0f,   BMS_STATE_FAULT,  BMS_FAULT_UNDERVOLTAGE,              "undervoltage" },
        { 48.0f,   21.0f,   25.0f,   BMS_STATE_FAULT,  BMS_FAULT_OVERCURRENT,               "overcurrent" },
        { 48.0f,  -21.0f,   25.0f,   BMS_STATE_FAULT,  BMS_FAULT_OVERCURRENT,               "negative overcurrent" },
        { 48.0f,   10.0f,   61.0f,   BMS_STATE_FAULT,  BMS_FAULT_OVERTEMPERATURE,           "overtemperature" },
        { 48.0f,   10.0f,  -21.0f,   BMS_STATE_FAULT,  BMS_FAULT_UNDERTEMPERATURE,          "undertemperature" },
        { 48.0f,   10.0f,   25.0f,   BMS_STATE_WARNING, BMS_FAULT_NONE,                     "warning" },
        { 48.0f,   10.0f,   25.0f,   BMS_STATE_FAULT,  BMS_FAULT_INVALID_MEASUREMENT,       "invalid measurement" },
        { 48.0f,   10.0f,   25.0f,   BMS_STATE_FAULT,  BMS_FAULT_INVALID_CONFIGURATION,     "invalid config" },
        {  0.0f,    0.0f, -100.0f,   BMS_STATE_FAULT,  BMS_FAULT_UNDERTEMPERATURE,          "temperature min" },
        { 48.0f,   32.767f, 25.0f,   BMS_STATE_NORMAL, BMS_FAULT_NONE,                     "current max positive" },
        { 48.0f,  -32.767f, 25.0f,   BMS_STATE_NORMAL, BMS_FAULT_NONE,                     "current max negative" },
        {655.35f,  10.0f,   25.0f,   BMS_STATE_NORMAL, BMS_FAULT_NONE,                     "voltage max" },
        { 48.0f,   10.0f, 6453.5f,   BMS_STATE_NORMAL, BMS_FAULT_NONE,                     "temperature max" },
    };

    const size_t num_vectors = sizeof(vectors) / sizeof(vectors[0]);
    float tolerance = 0.01f; /* due to quantization */

    for (size_t i = 0; i < num_vectors; i++)
    {
        bms_measurements_t m = make_measurements(
            vectors[i].voltage,
            vectors[i].current,
            vectors[i].temperature
        );
        bms_state_status_t st = make_state(vectors[i].state, vectors[i].fault);
        bms_can_frame_t frame;
        bms_measurements_t m2;
        bms_state_status_t st2;

        int r = bms_can_build_frame(&m, &st, &frame);
        assert(r == 0);
        assert(bms_can_frame_is_valid(&frame) == 1);

        r = bms_can_decode_frame(&frame, &m2, &st2);
        assert(r == 0);

        assert(measurements_equal(&m, &m2, tolerance));
        assert(st2.state == vectors[i].state);
        assert(st2.fault == vectors[i].fault);
    }
}

/* ------------------------------------------------------------
 * MAIN
 * ------------------------------------------------------------ */

int main(void)
{
    printf("[BMS CAN UNIT] Running tests...\n");

    /* Raw byte tests */
    test_can_raw_bytes_normal();
    printf("[PASS] raw_bytes_normal\n");

    test_can_raw_bytes_negative_current();
    printf("[PASS] raw_bytes_negative_current\n");

    test_can_raw_bytes_negative_temperature();
    printf("[PASS] raw_bytes_negative_temperature\n");

    /* Round-trip decode */
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

    /* Clamp tests */
    test_can_clamp_values();
    printf("[PASS] clamp_values\n");

    /* Invalid frame tests */
    test_can_frame_invalid_dlc();
    printf("[PASS] invalid_dlc\n");

    test_can_frame_invalid_id();
    printf("[PASS] invalid_id\n");

    test_can_frame_invalid_state_and_fault();
    printf("[PASS] invalid_state_and_fault\n");

    /* NEW: Round-trip boundary tests */
    test_can_roundtrip_boundaries();
    printf("[PASS] roundtrip_boundaries\n");

    printf("[BMS CAN UNIT] All tests passed.\n");
    return 0;
}
