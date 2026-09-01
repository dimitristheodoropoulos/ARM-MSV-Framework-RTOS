#ifndef BMS_MANAGER_H
#define BMS_MANAGER_H

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
    int limits_valid;                     /* 1 if limits passed validation, 0 otherwise */
    bms_protection_status_t protection;   /* Primary protection result */
    bms_fault_mask_t fault_mask;          /* All active protection faults */
    bms_state_status_t status;            /* Final system state + fault */
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
 *   2. Evaluates all protection conditions (fault_mask).
 *   3. Evaluates deterministic primary protection (protection).
 *   4. Updates system state.
 *
 * If the limits were invalid during initialisation, the update does nothing
 * and keeps the manager in a fault state with BMS_FAULT_INVALID_CONFIGURATION.
 *
 * @param manager      Pointer to manager context
 * @param measurements Pointer to new measurements
 */
void bms_manager_update(bms_manager_t *manager,
                        const bms_measurements_t *measurements);

#endif /* BMS_MANAGER_H */