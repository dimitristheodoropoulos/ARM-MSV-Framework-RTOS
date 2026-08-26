#include "bms_state.h"

void bms_state_init(bms_state_status_t *status)
{
    if (status == 0)
    {
        return;
    }

    status->state = BMS_STATE_INIT;
    status->fault = BMS_FAULT_NONE;
}

void bms_state_update(
    bms_state_status_t *status,
    bms_protection_status_t protection_status)
{
    if (status == 0)
    {
        return;
    }

    switch (protection_status)
    {
        case BMS_PROTECTION_NORMAL:
            status->state = BMS_STATE_NORMAL;
            status->fault = BMS_FAULT_NONE;
            break;

        case BMS_PROTECTION_INVALID_MEASUREMENT:
            status->state = BMS_STATE_FAULT;
            status->fault = BMS_FAULT_INVALID_MEASUREMENT;
            break;

        case BMS_PROTECTION_OVER_VOLTAGE:
            status->state = BMS_STATE_FAULT;
            status->fault = BMS_FAULT_OVERVOLTAGE;
            break;

        case BMS_PROTECTION_UNDER_VOLTAGE:
            status->state = BMS_STATE_FAULT;
            status->fault = BMS_FAULT_UNDERVOLTAGE;
            break;

        case BMS_PROTECTION_OVER_CURRENT:
            status->state = BMS_STATE_FAULT;
            status->fault = BMS_FAULT_OVERCURRENT;
            break;

        case BMS_PROTECTION_OVER_TEMPERATURE:
            status->state = BMS_STATE_FAULT;
            status->fault = BMS_FAULT_OVERTEMPERATURE;
            break;

        case BMS_PROTECTION_UNDER_TEMPERATURE:
            status->state = BMS_STATE_FAULT;
            status->fault = BMS_FAULT_UNDERTEMPERATURE;
            break;

        default:
            status->state = BMS_STATE_FAULT;
            status->fault = BMS_FAULT_INVALID_MEASUREMENT;
            break;
    }
}