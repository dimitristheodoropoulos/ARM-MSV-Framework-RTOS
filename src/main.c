#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "shell.h"
#include "health_monitor.h"
#include "tinyml.h"          /* <-- NEW: TinyML header */
#include <string.h>

#define REBOOT_MAGIC_WDT    0xDEADBEEF
#define REBOOT_MAGIC_FAULT  0xCAFEBABE
#define REBOOT_MAGIC_CLEAN  0x00000000

extern volatile uint8_t cli_is_typing;

/* Καθολικά Handles για πρόσβαση από το Shell */
TaskHandle_t xCLI_Handle = NULL;
TaskHandle_t xAI_Handle = NULL;
TaskHandle_t xWDT_Handle = NULL;

/* NEW: global variable for latest TinyML prediction */
volatile float latest_prediction = 0.0f;

typedef struct {
    uint32_t magic;
    uint32_t reset_count;
    char last_error[64];
} boot_log_t;

__attribute__((section(".noinit"))) static boot_log_t persistent_log;


void update_log(uint32_t magic, const char* msg) {
    persistent_log.magic = magic;
    persistent_log.reset_count++;
    strncpy(persistent_log.last_error, msg, 63);
    persistent_log.last_error[63] = '\0';
}

void __attribute__((used)) HardFault_Handler(void) {
    __asm volatile ("cpsid i");
    if (persistent_log.magic == REBOOT_MAGIC_CLEAN) {
        update_log(REBOOT_MAGIC_FAULT, "CPU HardFault (Invalid Access)");
    }
    uart_puts_safe("\r\n[KERNEL] EMERGENCY RESET...\r\n");
    *((volatile uint32_t *)0xE000ED0C) = 0x05FA0004;
    while(1);
}

void check_reboot_reason(void) {
    uart_puts_safe("\r\n[BOOT] Diagnostic Log (v2.3):\r\n");
    if (persistent_log.magic == REBOOT_MAGIC_WDT) {
        uart_puts_safe(" -> STATUS: WATCHDOG RECOVERY\r\n");
        uart_puts_safe(" -> CAUSE:  "); uart_puts_safe(persistent_log.last_error); uart_puts_safe("\r\n");
    } else if (persistent_log.magic == REBOOT_MAGIC_FAULT) {
        uart_puts_safe(" -> STATUS: HARD FAULT RECOVERY\r\n");
        uart_puts_safe(" -> CAUSE:  "); uart_puts_safe(persistent_log.last_error); uart_puts_safe("\r\n");
    } else {
        uart_puts_safe(" -> STATUS: CLEAN BOOT\r\n");
        persistent_log.reset_count = 0;
    }
    uart_puts_safe(" -> TOTAL RESETS: ");
    uart_putc((persistent_log.reset_count % 10) + '0');
    uart_puts_safe("\r\n");
    persistent_log.magic = REBOOT_MAGIC_CLEAN;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    update_log(REBOOT_MAGIC_FAULT, "Stack Overflow!");
    HardFault_Handler();
}

void vTaskWatchdogMonitor(void *pvParameters) {
    (void)pvParameters;

    health_monitor_init();

    for(;;) {
        if (!cli_is_typing) uart_putc('.');
        if (health_monitor_all_healthy()) {
            health_monitor_clear();
        } else {
            update_log(REBOOT_MAGIC_WDT, "Task Hang (Heartbeat Timeout)");
            vTaskDelay(pdMS_TO_TICKS(100));
            HardFault_Handler();
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void vTaskCLI(void *pvParameters) {
    (void)pvParameters;
    char cmd_buf[64];
    for(;;) {
        health_monitor_heartbeat(HEALTH_TASK_CLI);
        uart_puts_safe("rtos_msv> ");
        shell_readline(cmd_buf, 64);
        if (cmd_buf[0] != '\0') shell_process(cmd_buf);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vTaskPredictiveAI(void *pvParameters) {
    (void)pvParameters;   /* avoid unused parameter warning */

    for(;;) {
        health_monitor_heartbeat(HEALTH_TASK_AI);

        /* NEW: perform TinyML inference with a dummy sensor value */
        latest_prediction = ml_predict_next_temp(25.0f);   /* 25°C input */

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

int main(void) {
    uart_init();
    check_reboot_reason();

    /* Δημιουργία Tasks και ανάθεση στα Handles */
    xTaskCreate(vTaskCLI,             "CLI", 512, NULL, 2, &xCLI_Handle);
    xTaskCreate(vTaskPredictiveAI,    "AI",  256, NULL, 2, &xAI_Handle);
    xTaskCreate(vTaskWatchdogMonitor, "WDT", 256, NULL, 4, &xWDT_Handle);

    vTaskStartScheduler();
    return 0;
}