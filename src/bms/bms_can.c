#include "bms_can.h"

#include <stddef.h>
#include <string.h>

/* Default CAN ID for BMS frames (11-bit) */
static uint32_t bms_can_base_id = 0x100U;

/* Protocol constants */
#define CAN_DLC_BMS 8U

/* Scaling factors for fixed-point representation */
#define SCALE_VOLTAGE   100U    /* 1 V = 100 units, 0.01 V resolution */
#define SCALE_CURRENT   1000    /* 1 A = 1000 units, 1 mA resolution */
#define SCALE_TEMPERATURE 10    /* 1 °C = 10 units, 0.1 °C resolution */

/* Offset for signed temperature storage (allows negative temperatures) */
#define TEMPERATURE_OFFSET 1000

/* Limits */
#define VOLTAGE_MAX  65535U     /* 655.35 V max */
#define CURRENT_MAX  32767      /* ±32.767 A */
#define TEMPERATURE_ENCODE_MAX 65535U  /* 65535 units with offset */

/**
 * @brief Round a float to nearest integer with proper handling of negative values.
 */
static int32_t round_to_int(float value)
{
    if (value >= 0.0f)
        return (int32_t)(value + 0.5f);
    else
        return (int32_t)(value - 0.5f);
}

/**
 * @brief Clamp a value between min and max.
 */
static int32_t clamp_int32(int32_t value, int32_t min, int32_t max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int bms_can_set_base_id(uint32_t id)
{
    /* Validate CAN ID: must not be 0 and must be within valid range */
    if (id == 0U)
        return -1;

    /* Check if it's a valid 11-bit ID (0x000..0x7FF) or 29-bit (0x000..0x1FFFFFFF) */
    /* We accept both, but reject values outside 29-bit range */
    if (id > 0x1FFFFFFFU)
        return -1;

    bms_can_base_id = id;
    return 0;
}

int bms_can_build_frame(
    const bms_measurements_t *measurements,
    const bms_state_status_t *state,
    bms_can_frame_t *frame)
{
    if (measurements == NULL || state == NULL || frame == NULL)
    {
        return -1;
    }

    /* Clear frame */
    memset(frame, 0, sizeof(bms_can_frame_t));

    /* Set CAN ID and DLC */
    frame->id = bms_can_base_id;
    frame->dlc = CAN_DLC_BMS;

    /* --- Byte 0-1: Voltage (uint16, 0.01 V) --- */
    int32_t v = round_to_int(measurements->voltage.value * SCALE_VOLTAGE);
    v = clamp_int32(v, 0, VOLTAGE_MAX);
    uint16_t v_u16 = (uint16_t)v;
    frame->data[0] = (v_u16 >> 8) & 0xFF;
    frame->data[1] = v_u16 & 0xFF;

    /* --- Byte 2-3: Current (int16, 0.001 A, two's complement) --- */
    int32_t i = round_to_int(measurements->current.value * SCALE_CURRENT);
    i = clamp_int32(i, -CURRENT_MAX, CURRENT_MAX);
    int16_t i_s16 = (int16_t)i;
    uint16_t i_u16 = (uint16_t)i_s16; /* two's complement representation */
    frame->data[2] = (i_u16 >> 8) & 0xFF;
    frame->data[3] = i_u16 & 0xFF;

    /* --- Byte 4-5: Temperature (uint16, 0.1 °C, offset +1000) --- */
    int32_t t = round_to_int(measurements->temperature.value * SCALE_TEMPERATURE);
    t += TEMPERATURE_OFFSET;
    t = clamp_int32(t, 0, TEMPERATURE_ENCODE_MAX);
    uint16_t t_u16 = (uint16_t)t;
    frame->data[4] = (t_u16 >> 8) & 0xFF;
    frame->data[5] = t_u16 & 0xFF;

    /* --- Byte 6: BMS State (uint8) --- */
    frame->data[6] = (uint8_t)state->state;

    /* --- Byte 7: Fault Flags (uint8) --- */
    frame->data[7] = (uint8_t)state->fault;

    return 0;
}

int bms_can_decode_frame(
    const bms_can_frame_t *frame,
    bms_measurements_t *measurements,
    bms_state_status_t *state)
{
    if (frame == NULL || measurements == NULL || state == NULL)
    {
        return -1;
    }

    /* Validate frame */
    if (!bms_can_frame_is_valid(frame))
    {
        return -1;
    }

    /* --- Decode voltage --- */
    uint16_t v_u16 = ((uint16_t)frame->data[0] << 8) | frame->data[1];
    measurements->voltage.value = (float)v_u16 / SCALE_VOLTAGE;
    measurements->voltage.status = BMS_MEAS_VALID;

    /* --- Decode current --- */
    uint16_t i_u16 = ((uint16_t)frame->data[2] << 8) | frame->data[3];
    int16_t i_s16 = (int16_t)i_u16; /* two's complement to signed */
    measurements->current.value = (float)i_s16 / SCALE_CURRENT;
    measurements->current.status = BMS_MEAS_VALID;

    /* --- Decode temperature --- */
    uint16_t t_u16 = ((uint16_t)frame->data[4] << 8) | frame->data[5];
    int32_t t = (int32_t)t_u16 - TEMPERATURE_OFFSET;
    measurements->temperature.value = (float)t / SCALE_TEMPERATURE;
    measurements->temperature.status = BMS_MEAS_VALID;

    /* --- Decode state and fault --- */
    state->state = (bms_state_t)frame->data[6];
    state->fault = (bms_fault_t)frame->data[7];

    return 0;
}

int bms_can_frame_is_valid(const bms_can_frame_t *frame)
{
    if (frame == NULL)
        return 0;

    if (frame->dlc != CAN_DLC_BMS)
        return 0;

    if (frame->id != bms_can_base_id)
        return 0;

    return 1;
}