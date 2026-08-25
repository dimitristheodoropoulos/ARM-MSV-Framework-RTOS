#include "FreeRTOS.h"
#include "task.h"

#include "uart.h"
#include "shell.h"
#include "health_monitor.h"
#include "tinyml.h"
#include "system_clock.h"
#include "watchdog.h"

#include <string.h>
#include <stdint.h>


/* ------------------------------------------------------------
 * Persistent reboot diagnostics
 * ------------------------------------------------------------ */

#define REBOOT_MAGIC_WDT    0xDEADBEEFUL
#define REBOOT_MAGIC_FAULT  0xCAFEBABEU
#define REBOOT_MAGIC_CLEAN  0x00000000UL


extern volatile uint8_t cli_is_typing;


/* ------------------------------------------------------------
 * Global task handles
 * ------------------------------------------------------------ */

TaskHandle_t xCLI_Handle = NULL;
TaskHandle_t xAI_Handle  = NULL;
TaskHandle_t xWDT_Handle = NULL;


/* ------------------------------------------------------------
 * TinyML result
 * ------------------------------------------------------------ */

volatile float latest_prediction = 0.0f;


/* ------------------------------------------------------------
 * Persistent boot log
 * ------------------------------------------------------------ */

typedef struct
{
    uint32_t magic;
    uint32_t reset_count;
    char last_error[64];
} boot_log_t;


/*
 * .noinit is intentionally not initialized by startup code.
 *
 * Therefore it survives a software reset as long as the target
 * retains RAM contents across that reset.
 */
__attribute__((section(".noinit")))
static boot_log_t persistent_log;


/* ------------------------------------------------------------
 * Local diagnostic helpers
 * ------------------------------------------------------------ */

/*
 * Print an unsigned integer in decimal using only the UART
 * primitives already provided by uart.h.
 *
 * This deliberately avoids introducing a new public UART API
 * just for reboot diagnostics.
 */
static void print_uint_dec(uint32_t value)
{
    char buf[11];
    unsigned int i = 0U;

    if (value == 0U) {
        uart_putc('0');
        return;
    }

    while (value > 0U && i < sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (i > 0U) {
        uart_putc(buf[--i]);
    }
}


/* ------------------------------------------------------------
 * Diagnostic logging
 * ------------------------------------------------------------ */

void update_log(uint32_t magic, const char *msg)
{
    persistent_log.magic = magic;

    persistent_log.reset_count++;

    strncpy(
        persistent_log.last_error,
        msg,
        63
    );

    persistent_log.last_error[63] = '\0';
}


/* ------------------------------------------------------------
 * HardFault handler
 * ------------------------------------------------------------ */

void __attribute__((used)) HardFault_Handler(void)
{
    __asm volatile ("cpsid i");

    /*
     * Do not overwrite an already-recorded diagnostic reason.
     */
    if (persistent_log.magic == REBOOT_MAGIC_CLEAN) {

        update_log(
            REBOOT_MAGIC_FAULT,
            "CPU HardFault (Invalid Access)"
        );
    }

    uart_puts_safe(
        "\r\n[KERNEL] EMERGENCY RESET...\r\n"
    );

    /*
     * Cortex-M3 System Control Block AIRCR:
     *
     *     VECTKEY     = 0x5FA
     *     SYSRESETREQ = bit 2
     */
    *((volatile uint32_t *)0xE000ED0CUL) =
        0x05FA0004UL;

    while (1) {
        /* Wait for system reset. */
    }
}


/* ------------------------------------------------------------
 * Reboot reason
 * ------------------------------------------------------------ */

void check_reboot_reason(void)
{
    uart_puts_safe(
        "\r\n[BOOT] Diagnostic Log (v2.3):\r\n"
    );

    if (persistent_log.magic == REBOOT_MAGIC_WDT) {

        uart_puts_safe(
            " -> STATUS: WATCHDOG RECOVERY\r\n"
        );

        uart_puts_safe(
            " -> CAUSE:  "
        );

        uart_puts_safe(
            persistent_log.last_error
        );

        uart_puts_safe("\r\n");

    } else if (persistent_log.magic == REBOOT_MAGIC_FAULT) {

        uart_puts_safe(
            " -> STATUS: HARD FAULT RECOVERY\r\n"
        );

        uart_puts_safe(
            " -> CAUSE:  "
        );

        uart_puts_safe(
            persistent_log.last_error
        );

        uart_puts_safe("\r\n");

    } else {

        uart_puts_safe(
            " -> STATUS: CLEAN BOOT\r\n"
        );

        /*
         * A clean boot starts a new diagnostic sequence.
         */
        persistent_log.reset_count = 0U;
    }

    uart_puts_safe(
        " -> TOTAL RESETS: "
    );

    print_uint_dec(
        persistent_log.reset_count
    );

    uart_puts_safe("\r\n");

    /*
     * Clear the reason after it has been reported.
     */
    persistent_log.magic = REBOOT_MAGIC_CLEAN;
}


/* ------------------------------------------------------------
 * FreeRTOS stack overflow hook
 * ------------------------------------------------------------ */

void vApplicationStackOverflowHook(
    TaskHandle_t xTask,
    char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    update_log(
        REBOOT_MAGIC_FAULT,
        "Stack Overflow!"
    );

    HardFault_Handler();
}


/* ------------------------------------------------------------
 * Watchdog / health monitor task
 * ------------------------------------------------------------ */

void vTaskWatchdogMonitor(void *pvParameters)
{
    (void)pvParameters;

    /*
     * Health-monitor window:
     *
     *     5000 ms
     *
     * The hardware watchdog is configured slightly above the
     * health-monitor period so that the monitor has time to
     * detect a task failure and record the diagnostic reason
     * before the hardware watchdog performs the final reset.
     */
    health_monitor_init();

    watchdog_init(6000);

    for (;;) {

        if (!cli_is_typing) {
            uart_putc('.');
        }

        if (health_monitor_all_healthy()) {

            /*
             * All monitored tasks reported activity during the
             * current health window.
             *
             * Only now is it safe to feed the hardware watchdog.
             */
            watchdog_feed();

            health_monitor_clear();

        } else {

            /*
             * Do NOT feed the hardware watchdog.
             *
             * Record the diagnostic reason and allow the
             * hardware watchdog to perform the actual reset.
             */
            update_log(
                REBOOT_MAGIC_WDT,
                "Task Hang (Heartbeat Timeout)"
            );

            uart_puts_safe(
                "\r\n[WDT] Health monitor failure\r\n"
            );

            /*
             * Give the UART/diagnostic path a short scheduling
             * window, but deliberately do not feed the WDT.
             *
             * The 6-second hardware watchdog timeout will then
             * force the reset.
             */
            vTaskDelay(
                pdMS_TO_TICKS(100)
            );

            for (;;) {
                /*
                 * Intentionally stop feeding the hardware WDT.
                 *
                 * Hardware watchdog performs the final reset.
                 */
                vTaskDelay(
                    pdMS_TO_TICKS(100)
                );
            }
        }

        /*
         * Health-monitor window:
         *
         *     5000 ms
         *     500 ticks @ 100 Hz
         */
        vTaskDelay(
            pdMS_TO_TICKS(5000)
        );
    }
}

/* ------------------------------------------------------------
 * CLI task
 * ------------------------------------------------------------ */

void vTaskCLI(void *pvParameters)
{
    (void)pvParameters;

    char cmd_buf[64];

    for (;;) {

        health_monitor_heartbeat(
            HEALTH_TASK_CLI
        );

        uart_puts_safe(
            "rtos_msv> "
        );

        shell_readline(
            cmd_buf,
            sizeof(cmd_buf)
        );

        if (cmd_buf[0] != '\0') {
            shell_process(cmd_buf);
        }

        /*
         * 50 ms at configTICK_RATE_HZ = 100 Hz:
         *
         *     5 FreeRTOS ticks
         */
        vTaskDelay(
            pdMS_TO_TICKS(50)
        );
    }
}


/* ------------------------------------------------------------
 * Predictive AI task
 * ------------------------------------------------------------ */

void vTaskPredictiveAI(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {

        health_monitor_heartbeat(
            HEALTH_TASK_AI
        );

        latest_prediction =
            ml_predict_next_temp(25.0f);

        /*
         * 2000 ms at 100 Hz:
         *
         *     200 FreeRTOS ticks
         */
        vTaskDelay(
            pdMS_TO_TICKS(2000)
        );
    }
}


/* ------------------------------------------------------------
 * Main
 * ------------------------------------------------------------ */

int main(void)
{
    /*
     * --------------------------------------------------------
     * 1. Configure the LM3S6965/QEMU system clock.
     * --------------------------------------------------------
     *
     * The firmware clock contract is:
     *
     *     SYSTEM_CLOCK_HZ = 50 MHz
     *
     * system_clock_init() programs the LM3S system-control
     * registers before any timing-dependent subsystem starts.
     *
     * This is the hardware/QEMU clock configuration.
     */
    system_clock_init();


    /*
     * --------------------------------------------------------
     * 2. Initialize UART.
     * --------------------------------------------------------
     */
    uart_init();


    /*
     * --------------------------------------------------------
     * 3. Report persistent reboot diagnostics.
     * --------------------------------------------------------
     */
    check_reboot_reason();


    /*
     * --------------------------------------------------------
     * 4. Create application tasks.
     * --------------------------------------------------------
     */

    xTaskCreate(
        vTaskCLI,
        "CLI",
        512,
        NULL,
        2,
        &xCLI_Handle
    );

    xTaskCreate(
        vTaskPredictiveAI,
        "AI",
        256,
        NULL,
        2,
        &xAI_Handle
    );

    xTaskCreate(
        vTaskWatchdogMonitor,
        "WDT",
        256,
        NULL,
        4,
        &xWDT_Handle
    );


    /*
     * --------------------------------------------------------
     * 5. Start FreeRTOS.
     * --------------------------------------------------------
     *
     * FreeRTOS configuration:
     *
     *     configCPU_CLOCK_HZ = SYSTEM_CLOCK_HZ
     *                         = 50,000,000 Hz
     *
     *     configTICK_RATE_HZ = 100 Hz
     *
     * Therefore:
     *
     *     tick period = 1 / 100
     *                 = 10 ms
     *
     *     SysTick reload
     *         = 50,000,000 / 100 - 1
     *         = 499,999
     *
     * The Cortex-M3 FreeRTOS port configures SysTick using
     * these values.
     */
    vTaskStartScheduler();


    /*
     * The scheduler should never return.
     */
    return 0;
}
