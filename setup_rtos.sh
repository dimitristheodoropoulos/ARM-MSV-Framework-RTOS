#!/bin/bash

# Προσωρινός φάκελος για το download
TEMP_DIR="freertos_temp"
REPO_URL="https://github.com/FreeRTOS/FreeRTOS-Kernel.git"

echo "[1/4] Cloning FreeRTOS Kernel..."
git clone --depth 1 $REPO_URL $TEMP_DIR

echo "[2/4] Copying Core Source Files..."
cp $TEMP_DIR/*.c src/rtos/Source/
cp $TEMP_DIR/include/*.h src/rtos/include/

echo "[3/4] Copying Cortex-M3 Portable Files..."
cp $TEMP_DIR/portable/GCC/ARM_CM3/port.c src/rtos/portable/GCC/ARM_CM3/
cp $TEMP_DIR/portable/GCC/ARM_CM3/portmacro.h src/rtos/portable/GCC/ARM_CM3/

echo "[4/4] Copying Memory Manager (Heap 4)..."
cp $TEMP_DIR/portable/MemMang/heap_4.c src/rtos/portable/MemMang/

echo "Cleaning up..."
rm -rf $TEMP_DIR

echo "✅ FreeRTOS Source Files are ready!"