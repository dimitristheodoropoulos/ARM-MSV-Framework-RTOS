#ifndef BMS_CAN_H
#define BMS_CAN_H

#include <stdint.h>

#include "bms_measurements.h"
#include "bms_state.h"

/**
 * @brief Software representation of a CAN frame.
 *
 * This structure is transport‑independent. It can be used by
 * a CAN driver to send the frame over hardware, or by tests to
 * verify encoding/decoding.
 */
typedef struct
{
    uint32_t id;      /**< CAN identifier (11-bit or 29-bit) */
    uint8_t  dlc;     /**< Data length code (0..8) */
    uint8_t  data[8]; /**< Payload data */
} bms_can_frame_t;

/**
 * @brief Build a CAN frame from BMS data.
 *
 * Encodes the current BMS state, fault, and measurements into a CAN frame
 * according to the defined protocol.
 *
 * Protocol (8 bytes, DLC=8):
 *   Byte 0-1 : Voltage      uint16, 0.01 V resolution (0..655.35 V)
 *   Byte 2-3 : Current      int16,  0.001 A resolution (±32.767 A), two's complement
 *   Byte 4-5 : Temperature  uint16, 0.1 °C resolution, offset +1000
 *   Byte 6   : BMS State    uint8   (bms_state_t)
 *   Byte 7   : Fault Flags  uint8   (bms_fault_t)
 *
 * @param measurements   Pointer to current measurements (must not be NULL)
 * @param state          Pointer to current BMS state and fault (must not be NULL)
 * @param frame          Output CAN frame (must not be NULL)
 *
 * @return 0 on success, -1 on invalid input.
 */
int bms_can_build_frame(
    const bms_measurements_t *measurements,
    const bms_state_status_t *state,
    bms_can_frame_t *frame
);

/**
 * @brief Decode a CAN frame back into BMS data.
 *
 * This function is primarily intended for testing and validation
 * of the encoding scheme. It extracts measurements, state and fault
 * from a CAN frame.
 *
 * @param frame          Input CAN frame (must not be NULL)
 * @param measurements   Output measurements (must not be NULL)
 * @param state          Output BMS state and fault (must not be NULL)
 *
 * @return 0 on success, -1 on invalid input or malformed frame.
 */
int bms_can_decode_frame(
    const bms_can_frame_t *frame,
    bms_measurements_t *measurements,
    bms_state_status_t *state
);

/**
 * @brief Validate a CAN frame's integrity.
 *
 * Checks that the frame has a valid ID and DLC.
 *
 * @param frame  Input CAN frame (must not be NULL)
 *
 * @return 1 if valid, 0 if invalid.
 */
int bms_can_frame_is_valid(const bms_can_frame_t *frame);

/**
 * @brief Set the CAN identifier to be used for BMS frames.
 *
 * Allows reconfiguration of the CAN ID without changing the
 * encoding logic. Validates that the ID is within valid CAN range
 * (11-bit: 0x000..0x7FF, 29-bit: 0x000..0x1FFFFFFF).
 *
 * @param id  CAN identifier (11-bit or 29-bit)
 *
 * @return 0 on success, -1 if ID is invalid (e.g., 0x0 or out of range).
 */
int bms_can_set_base_id(uint32_t id);

#endif /* BMS_CAN_H */
