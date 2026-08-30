#!/usr/bin/env bash

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="/tmp/bms_static_analysis"

CC="${CC:-arm-none-eabi-gcc}"
CFLAGS="-std=c11 -Wall -Wextra -Werror -fanalyzer"
INCLUDES="-I${ROOT_DIR}/src/bms"

mkdir -p "${BUILD_DIR}"

passed=0
total=7

run_analysis()
{
    local name="$1"
    local source="$2"
    local output="${BUILD_DIR}/${name}.o"

    echo
    echo "[${name}]"

    if "${CC}" ${CFLAGS} ${INCLUDES} \
        -c "${ROOT_DIR}/${source}" \
        -o "${output}"
    then
        passed=$((passed + 1))
        echo "[PASS] ${name}"
    else
        echo "[FAIL] ${name}"
    fi
}

echo "========================================"
echo " BMS GCC STATIC ANALYSIS"
echo "========================================"

run_analysis "bms_measurements" \
    "src/bms/bms_measurements.c"

run_analysis "bms_state" \
    "src/bms/bms_state.c"

run_analysis "bms_protection" \
    "src/bms/bms_protection.c"

run_analysis "bms_limits" \
    "src/bms/bms_limits.c"

run_analysis "bms_manager" \
    "src/bms/bms_manager.c"

run_analysis "bms_measurement_device" \
    "src/bms/bms_measurement_device.c"

run_analysis "bms_can" \
    "src/bms/bms_can.c"

echo
echo "========================================"
echo " BMS STATIC ANALYSIS RESULT: ${passed}/${total} PASS"
echo "========================================"

if [ "${passed}" -ne "${total}" ]; then
    echo "[FAIL] Static analysis detected one or more failures."
    exit 1
else
    echo "[PASS] All BMS modules passed GCC static analysis."
    exit 0
fi
