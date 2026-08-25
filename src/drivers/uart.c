#include "uart.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define UART0_BASE 0x4000C000UL

#define UART_DR    (*(volatile unsigned char *)(UART0_BASE + 0x00UL))
#define UART_FR    (*(volatile unsigned int  *)(UART0_BASE + 0x18UL))
#define UART_IBRD  (*(volatile unsigned int  *)(UART0_BASE + 0x24UL))
#define UART_FBRD  (*(volatile unsigned int  *)(UART0_BASE + 0x28UL))
#define UART_LCRH  (*(volatile unsigned int  *)(UART0_BASE + 0x2CUL))
#define UART_CR    (*(volatile unsigned int  *)(UART0_BASE + 0x30UL))

/*
 * LM3S6965 / PL011-compatible UART
 *
 * System/UART clock:
 *
 *     50 MHz
 *
 * Target baud:
 *
 *     115200 baud
 *
 * Baud divisor:
 *
 *     50,000,000 / (16 * 115200)
 *       = 27.1267
 *
 * Therefore:
 *
 *     IBRD = 27
 *     FBRD = round(0.1267 * 64) = 8
 *
 * This gives approximately 115207 baud.
 */

#define UART_BAUD_IBRD  27UL
#define UART_BAUD_FBRD  8UL

SemaphoreHandle_t xUARTMutex = NULL;


/* ------------------------------------------------------------
 * UART initialization
 * ------------------------------------------------------------ */

void uart_init(void)
{
    /*
     * Disable UART before programming it.
     */
    UART_CR = 0UL;

    /*
     * 115200 baud @ 50 MHz UART clock.
     */
    UART_IBRD = UART_BAUD_IBRD;
    UART_FBRD = UART_BAUD_FBRD;

    /*
     * 8 data bits, no parity, one stop bit,
     * FIFO enabled.
     *
     * 0x60 = WLEN=8 + FEN.
     */
    UART_LCRH = 0x60UL;

    /*
     * UARTEN  = bit 8
     * TXE     = bit 9
     * RXE     = bit 0
     */
    UART_CR = 0x301UL;

    /*
     * Create the mutex once.
     */
    if (xUARTMutex == NULL) {
        xUARTMutex = xSemaphoreCreateMutex();
    }
}


/* ------------------------------------------------------------
 * UART transmit
 * ------------------------------------------------------------ */

void uart_putc(char c)
{
    /*
     * TXFF = bit 5
     *
     * Wait while transmit FIFO is full.
     */
    while (UART_FR & (1UL << 5)) {
        /* wait */
    }

    UART_DR = (unsigned char)c;
}


/* ------------------------------------------------------------
 * UART receive
 * ------------------------------------------------------------ */

char uart_getc(void)
{
    /*
     * RXFE = bit 4
     *
     * If RX FIFO is empty, return zero.
     * This keeps the shell polling non-blocking.
     */
    if (UART_FR & (1UL << 4)) {
        return 0;
    }

    return (char)UART_DR;
}


/* ------------------------------------------------------------
 * UART string output
 * ------------------------------------------------------------ */

void uart_puts(const char *s)
{
    if (s == NULL) {
        return;
    }

    while (*s != '\0') {
        uart_putc(*s++);
    }
}


/* ------------------------------------------------------------
 * Thread-safe UART string output
 * ------------------------------------------------------------ */

void uart_puts_safe(const char *s)
{
    if (s == NULL) {
        return;
    }

    /*
     * Before the scheduler starts, the mutex cannot safely
     * be used. Fall back to direct output.
     */
    if (xUARTMutex != NULL &&
        xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {

        if (xSemaphoreTake(
                xUARTMutex,
                portMAX_DELAY) == pdTRUE) {

            uart_puts(s);

            xSemaphoreGive(xUARTMutex);
        }

    } else {

        uart_puts(s);
    }
}


/* ------------------------------------------------------------
 * UART hexadecimal output
 * ------------------------------------------------------------ */

void uart_print_hex(unsigned int val)
{
    static const char hex[] =
        "0123456789ABCDEF";

    uart_puts("0x");

    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(
            hex[(val >> i) & 0xFUL]
        );
    }
}
