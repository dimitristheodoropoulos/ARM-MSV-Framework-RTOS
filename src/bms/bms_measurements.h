#ifndef BMS_MEASUREMENTS_H
#define BMS_MEASUREMENTS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BMS_MEAS_VALID = 0,
    BMS_MEAS_INVALID,
    BMS_MEAS_NOT_AVAILABLE,
    BMS_MEAS_OUT_OF_RANGE
} bms_measurement_status_t;

typedef struct
{
    float value;
    bms_measurement_status_t status;
} bms_measurement_t;

typedef struct
{
    bms_measurement_t voltage;
    bms_measurement_t current;
    bms_measurement_t temperature;
} bms_measurements_t;

/**
 * @brief Initialize a BMS measurement container.
 *
 * All measurements are initialized to zero and marked
 * as not available.
 *
 * @param measurements Pointer to measurement container.
 */
void bms_measurements_init(bms_measurements_t *measurements);

/**
 * @brief Validate the availability/status of all measurements.
 *
 * This function checks measurement status only. Physical
 * safety limits are handled by the protection layer.
 *
 * @param measurements Pointer to measurement container.
 *
 * @return 0 if all measurements are valid.
 * @return -1 if the pointer is NULL or any measurement
 *         is not valid.
 */
int bms_measurements_validate(const bms_measurements_t *measurements);

#ifdef __cplusplus
}
#endif

#endif /* BMS_MEASUREMENTS_H */