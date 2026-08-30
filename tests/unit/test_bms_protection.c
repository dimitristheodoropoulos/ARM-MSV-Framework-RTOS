#include <assert.h>
#include <stdio.h>

#include "bms_measurements.h"
#include "bms_protection.h"

static bms_measurements_t valid_measurements(void)
{
    bms_measurements_t m;
    bms_measurements_init(&m);
    m.voltage.value = 48.0f;
    m.voltage.status = BMS_MEAS_VALID;
    m.current.value = 10.0f;
    m.current.status = BMS_MEAS_VALID;
    m.temperature.value = 25.0f;
    m.temperature.status = BMS_MEAS_VALID;
    return m;
}

static bms_limits_t default_limits(void)
{
    bms_limits_t limits;
    limits.min_voltage = 40.0f;
    limits.max_voltage = 54.0f;
    limits.max_current = 20.0f;
    limits.min_temperature = -20.0f;
    limits.max_temperature = 60.0f;
    return limits;
}

int main(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();

    printf("[BMS PROTECTION UNIT] Running extended boundary tests...\n");

    /* ------------------------------------------------------------
     * Υπάρχουσες δοκιμές (βασικές)
     * ------------------------------------------------------------ */
    m = valid_measurements();
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    m.voltage.value = 55.0f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_OVER_VOLTAGE);

    m = valid_measurements();
    m.voltage.value = 39.0f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_UNDER_VOLTAGE);

    m = valid_measurements();
    m.current.value = 21.0f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_OVER_CURRENT);

    m = valid_measurements();
    m.temperature.value = 61.0f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_OVER_TEMPERATURE);

    m = valid_measurements();
    m.temperature.value = -21.0f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_UNDER_TEMPERATURE);

    assert(bms_protection_evaluate(NULL, &limits) == BMS_PROTECTION_INVALID_MEASUREMENT);
    assert(bms_protection_evaluate(&m, NULL) == BMS_PROTECTION_INVALID_MEASUREMENT);

    m.voltage.status = BMS_MEAS_NOT_AVAILABLE;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_INVALID_MEASUREMENT);

    /* ------------------------------------------------------------
     * Νέες δοκιμές οριακών τιμών
     * ------------------------------------------------------------ */

    /* Voltage: ακριβώς στα όρια */
    m = valid_measurements();
    m.voltage.value = 40.0f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    m.voltage.value = 54.0f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    /* Voltage: λίγο εντός */
    m.voltage.value = 40.0001f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    m.voltage.value = 53.9999f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    /* Voltage: λίγο εκτός */
    m.voltage.value = 39.9999f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_UNDER_VOLTAGE);

    m.voltage.value = 54.0001f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_OVER_VOLTAGE);

    /* Current: ακριβώς όριο */
    m = valid_measurements();
    m.current.value = 20.0f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    /* Current: λίγο εντός */
    m.current.value = 19.9999f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    /* Current: λίγο εκτός */
    m.current.value = 20.0001f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_OVER_CURRENT);

    /* Temperature: ακριβώς όρια */
    m = valid_measurements();
    m.temperature.value = -20.0f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    m.temperature.value = 60.0f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    /* Temperature: λίγο εντός */
    m.temperature.value = -19.9999f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    m.temperature.value = 59.9999f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_NORMAL);

    /* Temperature: λίγο εκτός */
    m.temperature.value = -20.0001f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_UNDER_TEMPERATURE);

    m.temperature.value = 60.0001f;
    assert(bms_protection_evaluate(&m, &limits) == BMS_PROTECTION_OVER_TEMPERATURE);


    /* ------------------------------------------------------------
     * BMS-REQ-049 — simultaneous multi-fault verification
     *
     * The protection interface reports one deterministic protection
     * status. When multiple physical fault conditions are active
     * simultaneously, the implementation must select the first
     * applicable condition according to its defined evaluation order:
     *
     *   over-voltage
     *   under-voltage
     *   over-current
     *   over-temperature
     *   under-temperature
     * ------------------------------------------------------------ */

    /* Over-voltage + over-current -> over-voltage has priority */
    m = valid_measurements();
    m.voltage.value = 55.0f;
    m.current.value = 21.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_VOLTAGE);

    /* Over-voltage + over-temperature -> over-voltage has priority */
    m = valid_measurements();
    m.voltage.value = 55.0f;
    m.temperature.value = 61.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_VOLTAGE);

    /* Under-voltage + over-current -> under-voltage has priority */
    m = valid_measurements();
    m.voltage.value = 39.0f;
    m.current.value = 21.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_UNDER_VOLTAGE);

    /* Under-voltage + under-temperature -> under-voltage has priority */
    m = valid_measurements();
    m.voltage.value = 39.0f;
    m.temperature.value = -21.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_UNDER_VOLTAGE);

    /* Over-current + over-temperature -> over-current has priority */
    m = valid_measurements();
    m.current.value = 21.0f;
    m.temperature.value = 61.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_CURRENT);

    /* Over-current + under-temperature -> over-current has priority */
    m = valid_measurements();
    m.current.value = 21.0f;
    m.temperature.value = -21.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_CURRENT);

    /* Over-temperature + under-temperature cannot coexist for one
     * scalar temperature measurement; verify the valid independent
     * multi-fault combinations instead of creating an impossible case. */

    printf("[PASS] BMS-REQ-049 simultaneous multi-fault combinations\n");

    printf("[BMS PROTECTION UNIT] All extended tests passed.\n");
    return 0;
}
