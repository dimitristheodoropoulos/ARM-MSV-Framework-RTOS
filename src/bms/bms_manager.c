#include "bms_manager.h"

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

    /* Run protection evaluation */
    manager->protection = bms_protection_evaluate(
        &manager->measurements,
        &manager->limits
    );

    /* Update system state based on protection result */
    bms_state_update(&manager->status, manager->protection);
}