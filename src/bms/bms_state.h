#ifndef BMS_STATE_H
#define BMS_STATE_H

#include "bms_protection.h"

typedef enum
{
    BMS_STATE_INIT = 0,
    BMS_STATE_NORMAL,
    BMS_STATE_WARNING,
    BMS_STATE_FAULT
} bms_state_t;

typedef enum
{
    BMS_FAULT_NONE = 0,
    BMS_FAULT_INVALID_MEASUREMENT,
    BMS_FAULT_OVERVOLTAGE,
    BMS_FAULT_UNDERVOLTAGE,
    BMS_FAULT_OVERTEMPERATURE,
    BMS_FAULT_UNDERTEMPERATURE,
    BMS_FAULT_OVERCURRENT
} bms_fault_t;

typedef struct
{
    bms_state_t state;
    bms_fault_t fault;
} bms_state_status_t;

void bms_state_init(bms_state_status_t *status);

void bms_state_update(
    bms_state_status_t *status,
    bms_protection_status_t protection_status
);

#endif /* BMS_STATE_H */