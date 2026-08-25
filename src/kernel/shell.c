#include "shell.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "nmea_parser.h"
#include "task.h"

/* NEW: access latest prediction from main.c */
extern volatile float latest_prediction;

volatile uint8_t cli_is_typing = 0;

/* Εισαγωγή των handles από τη main */
extern TaskHandle_t xAI_Handle;

/* * Βοηθητική συνάρτηση για εκτύπωση ακεραίων στη UART.
 * Μετατρέπει τον αριθμό σε string χαρακτήρα-χαρακτήρα.
 */
void uart_put_num(uint32_t num) {
    char buf[11];
    int i = 0;
    if (num == 0) {
        uart_putc('0');
        return;
    }
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    while (--i >= 0) {
        uart_putc(buf[i]);
    }
}

static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a == '\r' || *a == '\n') break;
        if (*a != *b) return 0;
        a++; b++;
    }
    return (*b == '\0');
}

void shell_readline(char *buf, int maxlen) {
    int i = 0;
    while (i < maxlen - 1) {
        char c = uart_getc();
        if (c == 0) {
            if (i == 0) cli_is_typing = 0;
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        cli_is_typing = 1;
        if (c == '\r' || c == '\n') {
            buf[i] = '\0';
            uart_puts_safe("\r\n");
            cli_is_typing = 0;
            return;
        }
        if (c == 8 || c == 127) {
            if (i > 0) { i--; uart_puts_safe("\b \b"); }
            continue;
        }
        if (c >= 32 && c <= 126) { buf[i++] = c; uart_putc(c); }
    }
    buf[i] = '\0';
}

void shell_process(char *cmd) {
    /* --- ΕΝΤΟΛΗ MEM (Heap Status) --- */
    if (str_eq(cmd, "mem")) {
        size_t free_heap = xPortGetFreeHeapSize();
        size_t min_ever = xPortGetMinimumEverFreeHeapSize();

        uart_puts_safe("\r\n--- HEAP MEMORY INFO ---\r\n");
        uart_puts_safe("Free now:      "); uart_put_num((uint32_t)free_heap); uart_puts_safe(" bytes\r\n");
        uart_puts_safe("Lifetime min:  "); uart_put_num((uint32_t)min_ever);  uart_puts_safe(" bytes\r\n");
    }

    /* --- ΕΝΤΟΛΗ UPTIME --- */
    else if (str_eq(cmd, "uptime")) {
        TickType_t ticks = xTaskGetTickCount();
        uint32_t total_sec = ticks / configTICK_RATE_HZ;
        uint32_t min = total_sec / 60;
        uint32_t sec = total_sec % 60;

        uart_puts_safe("\r\nSystem Uptime: ");
        uart_put_num(min); uart_puts_safe("m ");
        uart_put_num(sec); uart_puts_safe("s\r\n");
    }

    /* --- ΕΝΤΟΛΕΣ PRIORITY --- */
    else if (str_eq(cmd, "boost_ai")) {
        if (xAI_Handle != NULL) {
            vTaskPrioritySet(xAI_Handle, 4);
            uart_puts_safe("\r\n[SYSTEM] AI Priority boosted to 4\r\n");
        }
    }
    else if (str_eq(cmd, "low_ai")) {
        if (xAI_Handle != NULL) {
            vTaskPrioritySet(xAI_Handle, 1);
            uart_puts_safe("\r\n[SYSTEM] AI Priority lowered to 1\r\n");
        }
    }

    /* --- ΕΝΤΟΛΗ PS --- */
    else if (str_eq(cmd, "ps")) {
        char buffer[256];
        uart_puts_safe("\r\nName          State  Prio  Stack  Num\r\n");
        uart_puts_safe("-------------------------------------\r\n");
        vTaskList(buffer);
        uart_puts_safe(buffer);
    }

    /* --- ΕΝΤΟΛΗ HELP --- */
    else if (str_eq(cmd, "help")) {
        uart_puts_safe("\r\n--- ARM MSV CLI v2.4 ---\r\n");
        uart_puts_safe("ps       - Task Statistics\r\n");
        uart_puts_safe("mem      - Heap Memory Usage\r\n");
        uart_puts_safe("uptime   - System Uptime\r\n");
        uart_puts_safe("boost_ai - AI Task High Priority\r\n");
        uart_puts_safe("low_ai   - AI Task Low Priority\r\n");
        uart_puts_safe("freeze   - Force CLI Hang (WDT Test)\r\n");
        uart_puts_safe("crash    - Force HardFault\r\n");
        /* NEW: predict command */
        uart_puts_safe("predict  - TinyML Temperature Prediction\r\n");
        uart_puts_safe("nmea     - GNSS NMEA parser sample\r\n");
    }

    /* --- ΕΝΤΟΛΗ CRASH --- */
    else if (str_eq(cmd, "crash")) {
        uart_puts_safe("[TEST] Forcing Usage Fault...\r\n");
        vTaskDelay(pdMS_TO_TICKS(100));
        void (*bad_func)(void) = (void *)0x00000002;
        bad_func();
    }

    /* --- ΕΝΤΟΛΗ FREEZE --- */
    else if (str_eq(cmd, "freeze")) {
        uart_puts_safe("[TEST] Freezing CLI...\r\n");
        vTaskSuspend(NULL);
    }

    /* --- NEW: ΤinyML PREDICT --- */
    else if (str_eq(cmd, "nmea")) {
        nmea_print_sample();
    }

    else if (str_eq(cmd, "predict")) {
        uart_puts_safe("\r\n[TinyML] Predicted temperature: ");

        /* Print float as integer + fraction (no printf) */
        int whole = (int)latest_prediction;
        int frac  = (int)((latest_prediction - whole) * 100.0f);

        uart_put_num((uint32_t)whole);
        uart_putc('.');
        uart_putc('0' + ((frac / 10) % 10));
        uart_putc('0' + (frac % 10));
        uart_puts_safe(" C\r\n");
    }

    /* --- NEW: Unknown command fallback --- */
    else {
        uart_puts_safe("Unknown command\r\n");
    }
}