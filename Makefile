CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size


# Keep -O0 so fault-injection / invalid-memory-access tests
# remain observable.
CFLAGS  = -mcpu=cortex-m3 -mthumb -msoft-float
CFLAGS += -ffreestanding -g -O0
CFLAGS += --specs=nosys.specs --specs=nano.specs
CFLAGS += -nostartfiles -fno-common


LDFLAGS = -Tlinker.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -lc -lm -lgcc


SRC_DIR = src

RTOS_DIR          = $(SRC_DIR)/rtos
RTOS_SOURCE_DIR   = $(RTOS_DIR)/Source
RTOS_PORTABLE_DIR = $(RTOS_DIR)/portable/GCC/ARM_CM3
RTOS_MEMMANG_DIR  = $(RTOS_DIR)/portable/MemMang


INCLUDES = -I $(SRC_DIR)/arch/arm \
           -I $(SRC_DIR)/bms \
           -I $(SRC_DIR)/drivers \
           -I $(SRC_DIR)/kernel \
           -I $(SRC_DIR)/gnss \
           -I $(SRC_DIR)/utils \
           -I $(RTOS_DIR) \
           -I $(RTOS_DIR)/include \
           -I $(RTOS_PORTABLE_DIR) \
           -I .


SRCS = $(SRC_DIR)/arch/arm/syscalls.c \
       $(SRC_DIR)/arch/arm/system_clock.c \
       $(SRC_DIR)/bms/bms_measurements.c \
       $(SRC_DIR)/bms/bms_state.c \
       $(SRC_DIR)/bms/bms_protection.c \
       $(SRC_DIR)/bms/bms_manager.c \
       $(SRC_DIR)/drivers/uart.c \
       $(SRC_DIR)/drivers/gpio.c \
       $(SRC_DIR)/drivers/watchdog.c \
       $(SRC_DIR)/drivers/timer.c \
       $(SRC_DIR)/gnss/nmea_parser.c \
       $(SRC_DIR)/kernel/shell.c \
       $(SRC_DIR)/kernel/health_monitor.c \
       $(SRC_DIR)/main.c \
       $(RTOS_SOURCE_DIR)/tasks.c \
       $(RTOS_SOURCE_DIR)/queue.c \
       $(RTOS_SOURCE_DIR)/list.c \
       $(RTOS_SOURCE_DIR)/timers.c \
       $(RTOS_PORTABLE_DIR)/port.c \
       $(RTOS_MEMMANG_DIR)/heap_4.c


ASMS = $(SRC_DIR)/arch/arm/startup.s

OBJS = $(ASMS:.s=.o) $(SRCS:.c=.o)


all: firmware.elf


firmware.elf: $(OBJS)
	@echo "[LINKING] $@"
	@$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) $(LDFLAGS) -o $@
	@$(SIZE) $@


%.o: %.c
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


%.o: %.s
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


clean:
	rm -f $(OBJS) firmware.elf


BMS_TEST_DIR = build/tests
BMS_HOST_CFLAGS = -std=c11 -Wall -Wextra -Werror -Isrc/bms

bms-test:
	@mkdir -p $(BMS_TEST_DIR)
	@echo "[BMS TEST] Building host unit tests..."
	@gcc $(BMS_HOST_CFLAGS) tests/unit/test_bms_measurements.c \
		src/bms/bms_measurements.c -lm \
		-o $(BMS_TEST_DIR)/test_bms_measurements
	@gcc $(BMS_HOST_CFLAGS) tests/unit/test_bms_protection.c \
		src/bms/bms_protection.c src/bms/bms_measurements.c -lm \
		-o $(BMS_TEST_DIR)/test_bms_protection
	@gcc $(BMS_HOST_CFLAGS) tests/unit/test_bms_state.c \
		src/bms/bms_state.c -lm \
		-o $(BMS_TEST_DIR)/test_bms_state
	@gcc $(BMS_HOST_CFLAGS) tests/unit/test_bms_manager.c \
		src/bms/bms_manager.c src/bms/bms_protection.c \
		src/bms/bms_measurements.c src/bms/bms_state.c -lm \
		-o $(BMS_TEST_DIR)/test_bms_manager
	@echo "[BMS TEST] Running host unit tests..."
	@$(BMS_TEST_DIR)/test_bms_measurements
	@$(BMS_TEST_DIR)/test_bms_protection
	@$(BMS_TEST_DIR)/test_bms_state
	@$(BMS_TEST_DIR)/test_bms_manager
	@echo "[BMS TEST] All BMS unit tests passed."


.PHONY: all clean test verify bms-test

test: firmware.elf bms-test
	pytest -q

verify: clean
	$(MAKE) firmware.elf
	$(MAKE) test