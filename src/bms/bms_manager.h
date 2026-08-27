#ifndef BMS_MANAGER_H
#define BMS_MANAGER_H

#include <stdint.h>

#include "bms_measurements.h"
#include "bms_protection.h"
#include "bms_state.h"

/**
 * @brief BMS manager context.
 *
 * Orchestrates the entire BMS pipeline:
 *   measurements → protection evaluation → state update.
 */
typedef struct
{
    bms_measurements_t measurements;      /* Latest raw measurements */
    bms_limits_t limits;                  /* Operational limits */
    bms_protection_status_t protection;   /* Result of protection evaluation */
    bms_state_status_t status;            /* Final system state + fault */

    /**
     * Configuration validity is latched until reinitialisation.
     *
     * 0 = configuration is valid.
     * 1 = configuration is invalid (BMS_FAULT_INVALID_CONFIGURATION).
     *
     * Once set, this flag prevents the manager from ever transitioning
     * to NORMAL state, regardless of subsequent measurements.
     */
    uint8_t configuration_fault_latched;
} bms_manager_t;

/**
 * @brief Initialise the BMS manager.
 *
 * @param manager Pointer to manager context (must not be NULL)
 * @param limits  Pointer to limits to be used (must not be NULL)
 */
void bms_manager_init(bms_manager_t *manager, const bms_limits_t *limits);

/**
 * @brief Update the BMS with new measurements.
 *
 * Runs the full pipeline:
 *   1. Stores the measurements.
 *   2. Evaluates protection.
 *   3. Updates system state.
 *
 * @param manager      Pointer to manager context
 * @param measurements Pointer to new measurements
 */
void bms_manager_update(bms_manager_t *manager,
                        const bms_measurements_t *measurements);

#endif /* BMS_MANAGER_H */