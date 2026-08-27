#include "FreeRTOS.h"
#include "task.h"

#include "uart.h"
#include "shell.h"
#include "health_monitor.h"
#include "tinyml.h"
#include "system_clock.h"
#include "watchdog.h"

#include "bms_manager.h"   /* ΝΕΟ: BMS manager */

#include <string.h>
#include <stdint.h>


/* ------------------------------------------------------------
 * Watchdog timing constants
 * ------------------------------------------------------------ */

#define WDT_TIMEOUT_MS         10000U
#define WDT_MONITOR_PERIOD_MS   5000U


/* ------------------------------------------------------------
 * Persistent reboot diagnostics
 * ------------------------------------------------------------ */

#define REBOOT_MAGIC_WDT    0xDEADBEEFUL
#define REBOOT_MAGIC_FAULT  0xCAFEBABEU
#define REBOOT_MAGIC_CLEAN  0x00000000UL


extern volatile uint8_t cli_is_typing;


/* ------------------------------------------------------------
 * Global task handles (προστέθηκε xBMS_Handle)
 * ------------------------------------------------------------ */

TaskHandle_t xCLI_Handle = NULL;
TaskHandle_t xAI_Handle  = NULL;
TaskHandle_t xWDT_Handle = NULL;
TaskHandle_t xBMS_Handle = NULL;   /* ΝΕΟ */


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
 * BMS manager context (ΝΕΟ)
 * ------------------------------------------------------------ */

static bms_manager_t bms_manager;
static bms_limits_t bms_limits;


/* ------------------------------------------------------------
 * Local diagnostic helpers (υπάρχον)
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

/*
 * Print a signed integer in decimal using only the UART
 * primitives already provided by uart.h.
 */
static void print_int_dec(int32_t value)
{
    if (value < 0) {
        uart_putc('-');

        /*
         * Avoid signed overflow for INT32_MIN.
         */
        uint32_t magnitude =
            (uint32_t)(-(value + 1)) + 1U;

        print_uint_dec(magnitude);
        return;
    }

    print_uint_dec((uint32_t)value);
}


/* ------------------------------------------------------------
 * Diagnostic logging (υπάρχον)
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
 * HardFault handler (υπάρχον)
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
 * Reboot reason (υπάρχον)
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
 * FreeRTOS stack overflow hook (υπάρχον)
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
 * BMS diagnostics (ΝΕΟ)
 * ------------------------------------------------------------ */

static void print_bms_status(void)
{
    const bms_manager_t *mgr = &bms_manager;

    uart_puts_safe("\r\n========== BMS STATUS ==========\r\n");

    uart_puts_safe("  State      : ");
    switch (mgr->status.state)
    {
        case BMS_STATE_INIT:   uart_puts_safe("INIT\r\n"); break;
        case BMS_STATE_NORMAL: uart_puts_safe("NORMAL\r\n"); break;
        case BMS_STATE_WARNING:uart_puts_safe("WARNING\r\n"); break;
        case BMS_STATE_FAULT:  uart_puts_safe("FAULT\r\n"); break;
        default:               uart_puts_safe("UNKNOWN\r\n"); break;
    }

    uart_puts_safe("  Protection : ");
    switch (mgr->protection)
    {
        case BMS_PROTECTION_NORMAL:              uart_puts_safe("NORMAL\r\n"); break;
        case BMS_PROTECTION_OVER_VOLTAGE:        uart_puts_safe("OVER_VOLTAGE\r\n"); break;
        case BMS_PROTECTION_UNDER_VOLTAGE:       uart_puts_safe("UNDER_VOLTAGE\r\n"); break;
        case BMS_PROTECTION_OVER_CURRENT:        uart_puts_safe("OVER_CURRENT\r\n"); break;
        case BMS_PROTECTION_OVER_TEMPERATURE:    uart_puts_safe("OVER_TEMPERATURE\r\n"); break;
        case BMS_PROTECTION_UNDER_TEMPERATURE:   uart_puts_safe("UNDER_TEMPERATURE\r\n"); break;
        case BMS_PROTECTION_INVALID_MEASUREMENT: uart_puts_safe("INVALID_MEASUREMENT\r\n"); break;
        default:                                 uart_puts_safe("UNKNOWN\r\n"); break;
    }

    uart_puts_safe("  Measurements:\r\n");

    uart_puts_safe("    Voltage   : ");
    print_uint_dec((uint32_t)(mgr->measurements.voltage.value * 100.0f));
    uart_puts_safe(" cV\r\n");

    uart_puts_safe("    Current   : ");
    print_uint_dec((uint32_t)(mgr->measurements.current.value * 1000.0f));
    uart_puts_safe(" mA\r\n");

    uart_puts_safe("    Temp.     : ");
    print_int_dec(
        (int32_t)(mgr->measurements.temperature.value * 10.0f)
    );
    uart_puts_safe(" x10 C\r\n");

    uart_puts_safe("================================\r\n");
}


/* ------------------------------------------------------------
 * BMS mock measurement (ΝΕΟ)
 * ------------------------------------------------------------ */

static bms_measurements_t make_bms_measurement(
    float voltage,
    float current,
    float temperature)
{
    bms_measurements_t m;

    bms_measurements_init(&m);

    m.voltage.value = voltage;
    m.voltage.status = BMS_MEAS_VALID;

    m.current.value = current;
    m.current.status = BMS_MEAS_VALID;

    m.temperature.value = temperature;
    m.temperature.status = BMS_MEAS_VALID;

    return m;
}


/* ------------------------------------------------------------
 * BMS task (ΝΕΟ)
 * ------------------------------------------------------------ */

void vTaskBMS(void *pvParameters)
{
    (void)pvParameters;

    typedef struct
    {
        float voltage;
        float current;
        float temperature;
        const char *description;
    } bms_test_vector_t;

    static const bms_test_vector_t vectors[] = {
        {48.0f, 10.0f,  25.0f, "Normal"},
        {55.0f, 10.0f,  25.0f, "Overvoltage"},
        {39.0f, 10.0f,  25.0f, "Undervoltage"},
        {48.0f, 21.0f,  25.0f, "Overcurrent"},
        {48.0f, 10.0f,  61.0f, "Overtemperature"},
        {48.0f, 10.0f, -21.0f, "Undertemperature"},
        {48.0f, 10.0f,  25.0f, "Normal again"}
    };

    const size_t num_vectors = sizeof(vectors) / sizeof(vectors[0]);
    size_t index = 0U;

    for (;;)
    {
        health_monitor_heartbeat(HEALTH_TASK_BMS);

        const bms_test_vector_t *vector = &vectors[index];

        bms_measurements_t measurement = make_bms_measurement(
            vector->voltage,
            vector->current,
            vector->temperature
        );

        uart_puts_safe("\r\n[BMS] Update: ");
        uart_puts_safe(vector->description);
        uart_puts_safe("\r\n");

        bms_manager_update(&bms_manager, &measurement);
        print_bms_status();

        index++;
        if (index >= num_vectors) {
            index = 0U;
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


/* ------------------------------------------------------------
 * Watchdog / health monitor task (με βελτιωμένο monitoring period)
 * ------------------------------------------------------------ */

void vTaskWatchdogMonitor(void *pvParameters)
{
    (void)pvParameters;

    health_monitor_init();

    watchdog_init(WDT_TIMEOUT_MS);

    for (;;)
    {
        if (!cli_is_typing) {
            uart_putc('.');
        }

        if (health_monitor_all_healthy())
        {
            watchdog_feed();
            health_monitor_clear();
        }
        else
        {
            update_log(REBOOT_MAGIC_WDT, "Task Hang (Heartbeat Timeout)");
            uart_puts_safe("\r\n[WDT] Health monitor failure\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));

            for (;;) {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(WDT_MONITOR_PERIOD_MS));
    }
}


/* ------------------------------------------------------------
 * CLI task (υπάρχον, αμετάβλητο)
 * ------------------------------------------------------------ */

void vTaskCLI(void *pvParameters)
{
    (void)pvParameters;

    char cmd_buf[64];

    for (;;)
    {
        health_monitor_heartbeat(HEALTH_TASK_CLI);

        uart_puts_safe("rtos_msv> ");
        shell_readline(cmd_buf, sizeof(cmd_buf));

        if (cmd_buf[0] != '\0') {
            shell_process(cmd_buf);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


/* ------------------------------------------------------------
 * Predictive AI task (υπάρχον, αμετάβλητο)
 * ------------------------------------------------------------ */

void vTaskPredictiveAI(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        health_monitor_heartbeat(HEALTH_TASK_AI);
        latest_prediction = ml_predict_next_temp(25.0f);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


/* ------------------------------------------------------------
 * Main
 * ------------------------------------------------------------ */

int main(void)
{
    /* 1. System clock */
    system_clock_init();

    /* 2. UART */
    uart_init();

    /* 3. Reboot diagnostics */
    check_reboot_reason();

    /* 4. BMS initialisation (ΝΕΟ) */
    bms_limits.min_voltage = 40.0f;
    bms_limits.max_voltage = 54.0f;
    bms_limits.max_current = 20.0f;
    bms_limits.min_temperature = -20.0f;
    bms_limits.max_temperature = 60.0f;

    bms_manager_init(&bms_manager, &bms_limits);
    uart_puts_safe("[BMS] Manager initialised.\r\n");

    /* 5. Create tasks */
    xTaskCreate(vTaskCLI, "CLI", 512, NULL, 2, &xCLI_Handle);
    xTaskCreate(vTaskPredictiveAI, "AI", 256, NULL, 2, &xAI_Handle);
    xTaskCreate(vTaskWatchdogMonitor, "WDT", 256, NULL, 4, &xWDT_Handle);
    xTaskCreate(vTaskBMS, "BMS", 384, NULL, 1, &xBMS_Handle);  /* ΝΕΟ */

    /* 6. Start scheduler */
    vTaskStartScheduler();

    return 0;
}