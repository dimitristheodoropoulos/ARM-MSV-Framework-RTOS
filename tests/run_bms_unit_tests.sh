#!/usr/bin/env bash

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="/tmp/bms_unit_tests"

CC="${CC:-gcc}"
CFLAGS="-std=c11 -Wall -Wextra -Werror"
INCLUDES="-I${ROOT_DIR}/src/bms"

mkdir -p "${BUILD_DIR}"

passed=0
total=6

run_test()
{
    local name="$1"
    local output="$2"
    shift 2

    echo
    echo "[${name}]"

    if "${CC}" ${CFLAGS} ${INCLUDES} "$@" -lm -o "${BUILD_DIR}/${output}"; then
        if "${BUILD_DIR}/${output}"; then
            passed=$((passed + 1))
            echo "[PASS] ${name}"
        else
            echo "[FAIL] ${name}: execution failed"
            return 1
        fi
    else
        echo "[FAIL] ${name}: compilation failed"
        return 1
    fi
}

echo "========================================"
echo " BMS UNIT REGRESSION"
echo "========================================"

run_test \
    "BMS measurements" \
    "test_bms_measurements" \
    "${ROOT_DIR}/tests/unit/test_bms_measurements.c" \
    "${ROOT_DIR}/src/bms/bms_measurements.c" || exit 1

run_test \
    "BMS limits" \
    "test_bms_limits" \
    "${ROOT_DIR}/tests/unit/test_bms_limits.c" \
    "${ROOT_DIR}/src/bms/bms_limits.c" || exit 1

run_test \
    "BMS protection" \
    "test_bms_protection" \
    "${ROOT_DIR}/tests/unit/test_bms_protection.c" \
    "${ROOT_DIR}/src/bms/bms_measurements.c" \
    "${ROOT_DIR}/src/bms/bms_limits.c" \
    "${ROOT_DIR}/src/bms/bms_protection.c" || exit 1

run_test \
    "BMS state" \
    "test_bms_state" \
    "${ROOT_DIR}/tests/unit/test_bms_state.c" \
    "${ROOT_DIR}/src/bms/bms_state.c" || exit 1

run_test \
    "BMS manager" \
    "test_bms_manager" \
    "${ROOT_DIR}/tests/unit/test_bms_manager.c" \
    "${ROOT_DIR}/src/bms/bms_measurements.c" \
    "${ROOT_DIR}/src/bms/bms_limits.c" \
    "${ROOT_DIR}/src/bms/bms_protection.c" \
    "${ROOT_DIR}/src/bms/bms_state.c" \
    "${ROOT_DIR}/src/bms/bms_manager.c" || exit 1

run_test \
    "BMS I2C abstraction" \
    "test_bms_i2c" \
    "${ROOT_DIR}/tests/unit/test_bms_i2c.c" \
    "${ROOT_DIR}/src/bms/bms_measurement_device.c" \
    "${ROOT_DIR}/src/bms/bms_measurements.c" || exit 1

echo
echo "========================================"
echo " BMS REGRESSION RESULT: ${passed}/${total} PASS"
echo "========================================"

if [ "${passed}" -eq "${total}" ]; then
    exit 0
fi

exit 1