#include "bms_manager.h"
#include "bms_limits.h"

void bms_manager_init(bms_manager_t *manager, const bms_limits_t *limits)
{
    if (manager == 0 || limits == 0)
    {
        return;
    }

    /* Initialise measurements with invalid status */
    bms_measurements_init(&manager->measurements);

    /* By default, configuration is valid */
    manager->configuration_fault_latched = 0U;

    /* Validate the protection limits */
    if (bms_limits_validate(limits) != 0)
    {
        /*
         * Invalid configuration – enter fail-safe fault state.
         * Store the limits for diagnostic visibility even though
         * they are not accepted as a valid configuration.
         */
        manager->limits = *limits;
        manager->protection = BMS_PROTECTION_INVALID_MEASUREMENT;

        bms_state_init(&manager->status);
        manager->status.state = BMS_STATE_FAULT;
        manager->status.fault = BMS_FAULT_INVALID_CONFIGURATION;

        /* Latch the invalid configuration */
        manager->configuration_fault_latched = 1U;
        return;
    }

    /* Valid configuration – normal initialisation */
    manager->limits = *limits;
    manager->protection = BMS_PROTECTION_INVALID_MEASUREMENT;

    bms_state_init(&manager->status);
}

void bms_manager_update(bms_manager_t *manager,
                        const bms_measurements_t *measurements)
{
    if (manager == 0 || measurements == 0)
    {
        return;
    }

    /* Store measurements */
    manager->measurements = *measurements;

    /*
     * If the configuration was invalid at initialisation,
     * the manager must remain in FAULT / INVALID_CONFIGURATION
     * and must not process measurements.
     *
     * Only a fresh initialisation with valid limits can clear
     * this latch.
     */
    if (manager->configuration_fault_latched != 0U)
    {
        manager->protection = BMS_PROTECTION_INVALID_MEASUREMENT;
        manager->status.state = BMS_STATE_FAULT;
        manager->status.fault = BMS_FAULT_INVALID_CONFIGURATION;
        return;
    }

    /* Run protection evaluation */
    manager->protection = bms_protection_evaluate(
        &manager->measurements,
        &manager->limits
    );

    /* Update system state based on protection result */
    bms_state_update(&manager->status, manager->protection);
}