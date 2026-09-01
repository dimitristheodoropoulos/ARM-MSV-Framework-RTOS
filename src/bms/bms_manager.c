#include "bms_manager.h"
#include "bms_limits.h"

void bms_manager_init(bms_manager_t *manager, const bms_limits_t *limits)
{
    if (manager == 0 || limits == 0)
    {
        return;
    }

    /* Αρχικοποίηση measurements με invalid status */
    bms_measurements_init(&manager->measurements);

    manager->limits = *limits;
    manager->protection = BMS_PROTECTION_INVALID_MEASUREMENT;
    manager->fault_mask = BMS_FAULT_MASK_NONE;
    manager->limits_valid = (bms_limits_validate(limits) == 0) ? 1 : 0;

    bms_state_init(&manager->status);

    /*
     * Invalid protection configuration shall not be accepted by the
     * BMS manager. Keep the manager in a safe fault state and prevent
     * protection evaluation against an invalid configuration.
     */
    if (manager->limits_valid == 0)
    {
        manager->protection = BMS_PROTECTION_INVALID_MEASUREMENT;
        manager->fault_mask = BMS_FAULT_MASK_NONE;
        manager->status.state = BMS_STATE_FAULT;
        manager->status.fault = BMS_FAULT_INVALID_CONFIGURATION;   /* ← semantic fix */
    }
}

void bms_manager_update(bms_manager_t *manager,
                        const bms_measurements_t *measurements)
{
    if (manager == 0 || measurements == 0)
    {
        return;
    }

    /*
     * Do not run the protection pipeline when the configured limits
     * were rejected during manager initialization.
     */
    if (manager->limits_valid == 0)
    {
        return;
    }

    /* Store measurements */
    manager->measurements = *measurements;

    /* Evaluate ALL protection conditions → multi‑fault mask */
    manager->fault_mask = bms_protection_evaluate_faults(
        &manager->measurements,
        &manager->limits
    );

    /* Evaluate deterministic primary protection status */
    manager->protection = bms_protection_evaluate(
        &manager->measurements,
        &manager->limits
    );

    /* Update system state based on primary protection result */
    bms_state_update(&manager->status, manager->protection);
}