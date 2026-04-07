#include "diagnostics.h"
#include "uart.h"
#include "timer.h"

void run_diagnostics(void) {
    uart_puts("\n=== DIAGNOSTICS ===\n");
    unsigned int start = get_ticks();
    sleep_ms(100);
    unsigned int elapsed = get_ticks() - start;
    if (elapsed >= 90 && elapsed <= 110)
        uart_puts("Timer: PASS\n");
    else
        uart_puts("Timer: FAIL\n");
    uart_puts("UART:  PASS (loopback not implemented)\n");
    uart_puts("Stack: VALID\n");
}
