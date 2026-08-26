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

bms_protection_status_t bms_protection_evaluate(
    const bms_measurements_t *measurements,
    const bms_limits_t *limits
);

#endif /* BMS_PROTECTION_H */
