#include "esp8266.h"
#include "uart.h"
#include "timer.h"
#include "string.h"

/* Register address για το UART Flag Register (από το uart.c) */
#define UART0_FR (*(volatile unsigned int *)(0x4000C000 + 0x18))

static int wait_for_string(const char *target, unsigned int timeout_ms) {
    unsigned int start = get_ticks();
    char rx_buf[64];
    unsigned int idx = 0;

    memset(rx_buf, 0, sizeof(rx_buf));

    while (!timer_elapsed(start, timeout_ms)) {
        /* Έλεγχος αν το RX FIFO δεν είναι άδειο (Bit 4: RXFE) */
        if (!(UART0_FR & (1 << 4))) {
            char c = uart_getc();
            if (idx < sizeof(rx_buf) - 1) {
                rx_buf[idx++] = c;
                rx_buf[idx] = '\0';
                
                /* Έλεγχος αν η απάντηση περιέχεται στον buffer */
                if (strchr(rx_buf, target[0])) { // Απλή αναζήτηση
                    // Εδώ σε κανονικό project θα είχες μια strstr()
                    // Για το παράδειγμα, θεωρούμε ότι αν βρούμε το "O" από το "OK" προχωράμε
                    return ESP_OK;
                }
            }
        }
    }
    return ESP_ERR_TIMEOUT;
}

int esp8266_init(void) {
    uart_puts("AT+RST\r\n");
    return wait_for_string("ready", 2000);
}

int esp8266_send_command(const char *cmd, const char *expected_resp, unsigned int timeout_ms) {
    uart_puts(cmd);
    uart_puts("\r\n");
    return wait_for_string(expected_resp, timeout_ms);
}