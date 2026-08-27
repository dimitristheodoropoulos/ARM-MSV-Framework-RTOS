#ifndef BMS_LIMITS_H
#define BMS_LIMITS_H

#include "bms_protection.h"

/**
 * @brief Validate a BMS protection-limit configuration.
 *
 * A valid configuration shall contain finite protection limits with
 * strictly ordered voltage and temperature ranges and a positive
 * maximum current.
 *
 * @param limits Pointer to protection-limit configuration.
 *
 * @return 0 if the configuration is valid.
 * @return -1 if the pointer is NULL or the configuration is invalid.
 */
int bms_limits_validate(const bms_limits_t *limits);

#endif /* BMS_LIMITS_H */
