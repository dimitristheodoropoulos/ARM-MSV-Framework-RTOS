CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# Χρησιμοποιούμε -O0 για να μην "εξαφανίσει" ο compiler το παράνομο memory access
CFLAGS  = -mcpu=cortex-m3 -mthumb -msoft-float -ffreestanding -g -O0
CFLAGS += --specs=nosys.specs --specs=nano.specs -nostartfiles -fno-common

LDFLAGS = -Tlinker.ld -Wl,--gc-sections -lc -lm -lgcc

SRC_DIR = src
RTOS_DIR = $(SRC_DIR)/rtos
RTOS_SOURCE_DIR = $(RTOS_DIR)/Source
RTOS_PORTABLE_DIR = $(RTOS_DIR)/portable/GCC/ARM_CM3
RTOS_MEMMANG_DIR = $(RTOS_DIR)/portable/MemMang

INCLUDES = -I $(SRC_DIR)/drivers -I $(SRC_DIR)/kernel -I $(SRC_DIR)/gnss \
           -I $(SRC_DIR)/utils -I $(RTOS_DIR) -I $(RTOS_DIR)/include \
           -I $(RTOS_PORTABLE_DIR) -I .

SRCS = $(SRC_DIR)/arch/arm/syscalls.c \
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