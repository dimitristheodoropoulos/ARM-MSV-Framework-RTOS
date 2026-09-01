#ifndef BMS_PROTECTION_H
#define BMS_PROTECTION_H

#include "bms_measurements.h"

typedef enum
{
    BMS_PROTECTION_NORMAL = 0,
    BMS_PROTECTION_OVER_VOLTAGE,
    BMS_PROTECTION_UNDER_VOLTAGE,
    BMS_PROTECTION_OVER_CURRENT,
    BMS_PROTECTION_OVER_TEMPERATURE,
    BMS_PROTECTION_UNDER_TEMPERATURE,
    BMS_PROTECTION_INVALID_MEASUREMENT
} bms_protection_status_t;

typedef struct
{
    float min_voltage;
    float max_voltage;
    float max_current;
    float min_temperature;
    float max_temperature;
} bms_limits_t;

/*
 * Multi-fault representation.
 *
 * Each protection condition has an independent bit, allowing multiple
 * simultaneously active protection conditions to be represented.
 */
typedef unsigned int bms_fault_mask_t;

#define BMS_FAULT_MASK_NONE              0u
#define BMS_FAULT_MASK_OVER_VOLTAGE      (1u << 0)
#define BMS_FAULT_MASK_UNDER_VOLTAGE     (1u << 1)
#define BMS_FAULT_MASK_OVER_CURRENT      (1u << 2)
#define BMS_FAULT_MASK_OVER_TEMPERATURE  (1u << 3)
#define BMS_FAULT_MASK_UNDER_TEMPERATURE (1u << 4)

/*
 * Evaluate all protection conditions simultaneously.
 *
 * Returns BMS_FAULT_MASK_NONE when all measurements are within limits.
 * Returns all simultaneously active protection conditions as a bitmask.
 *
 * Invalid input or invalid measurements are reported through the
 * existing BMS_PROTECTION_INVALID_MEASUREMENT status API.
 */
bms_fault_mask_t bms_protection_evaluate_faults(
    const bms_measurements_t *measurements,
    const bms_limits_t *limits
);

/*
 * Evaluate the protection conditions and return the deterministic
 * primary protection status. This legacy API is retained for
 * compatibility with the existing state/manager pipeline.
 */
bms_protection_status_t bms_protection_evaluate(
    const bms_measurements_t *measurements,
    const bms_limits_t *limits
);

#endif /* BMS_PROTECTION_H */
