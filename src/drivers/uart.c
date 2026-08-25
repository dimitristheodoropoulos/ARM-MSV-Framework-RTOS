#include "uart.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <stdint.h>

/* ------------------------------------------------------------
 * LM3S6965 UART0 / PL011-compatible register map
 * ------------------------------------------------------------ */

#define UART0_BASE 0x4000C000UL

#define UART_DR    (*(volatile unsigned char *)(UART0_BASE + 0x00UL))
#define UART_FR    (*(volatile unsigned int  *)(UART0_BASE + 0x18UL))
#define UART_IBRD  (*(volatile unsigned int  *)(UART0_BASE + 0x24UL))
#define UART_FBRD  (*(volatile unsigned int  *)(UART0_BASE + 0x28UL))
#define UART_LCRH  (*(volatile unsigned int  *)(UART0_BASE + 0x2CUL))
#define UART_CR    (*(volatile unsigned int  *)(UART0_BASE + 0x30UL))
#define UART_IFLS  (*(volatile unsigned int  *)(UART0_BASE + 0x34UL))
#define UART_IMSC  (*(volatile unsigned int  *)(UART0_BASE + 0x38UL))
#define UART_RIS   (*(volatile unsigned int  *)(UART0_BASE + 0x3CUL))
#define UART_MIS   (*(volatile unsigned int  *)(UART0_BASE + 0x40UL))
#define UART_ICR   (*(volatile unsigned int  *)(UART0_BASE + 0x44UL))

/* ------------------------------------------------------------
 * Cortex-M3 NVIC
 *
 * LM3S6965:
 *
 *     IRQ 5 = UART0
 *     Vector = exception 21
 * ------------------------------------------------------------ */

#define NVIC_ISER0 (*(volatile unsigned int *)0xE000E100UL)

#define UART0_IRQ 5U

/* ------------------------------------------------------------
 * UART interrupt bits
 * ------------------------------------------------------------ */

#define UART_INT_RX  (1UL << 4)
#define UART_INT_RT  (1UL << 6)

/* ------------------------------------------------------------
 * UART status bits
 * ------------------------------------------------------------ */

#define UART_FR_RXFE (1UL << 4)
#define UART_FR_TXFF (1UL << 5)

/* ------------------------------------------------------------
 * RX ring buffer
 *
 * 256 bytes allows power-of-two indexing.
 *
 * One slot is intentionally left unused so that:
 *
 *     head == tail       => empty
 *     next(head) == tail => full
 * ------------------------------------------------------------ */

#define UART_RX_BUFFER_SIZE 256U
#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1U)

static volatile unsigned char uart_rx_buffer[UART_RX_BUFFER_SIZE];

/*
 * Producer:
 *     UART0_IRQHandler()
 *
 * Consumer:
 *     uart_getc()
 *
 * Only the ISR modifies head.
 * Only the task modifies tail.
 */
static volatile uint32_t uart_rx_head = 0U;
static volatile uint32_t uart_rx_tail = 0U;

SemaphoreHandle_t xUARTMutex = NULL;


/* ------------------------------------------------------------
 * RX ring buffer
 * ------------------------------------------------------------ */

static void uart_rx_push(unsigned char c)
{
    uint32_t head;
    uint32_t next;

    head = uart_rx_head;

    next = (head + 1U) & UART_RX_BUFFER_MASK;

    /*
     * Buffer full.
     *
     * Drop the newest byte rather than overwrite unread data.
     */
    if (next == uart_rx_tail) {
        return;
    }

    uart_rx_buffer[head] = c;

    /*
     * Publish only after the byte has been stored.
     */
    uart_rx_head = next;
}


/* ------------------------------------------------------------
 * UART0 IRQ handler
 *
 * IMPORTANT:
 *
 * No FreeRTOS API is called from this ISR.
 * No mutex is taken.
 * No task is blocked.
 * ------------------------------------------------------------ */

void UART0_IRQHandler(void)
{
    unsigned int status;

    status = UART_MIS;

    /*
     * Handle receive interrupt and receive-timeout interrupt.
     */
    if ((status & (UART_INT_RX | UART_INT_RT)) != 0UL) {

        /*
         * Transfer every byte currently present in the
         * hardware RX FIFO into the software ring buffer.
         */
        while ((UART_FR & UART_FR_RXFE) == 0UL) {
            uart_rx_push(UART_DR);
        }

        /*
         * Clear the interrupt sources after draining the FIFO.
         */
        UART_ICR = UART_INT_RX | UART_INT_RT;
    }
}


/* ------------------------------------------------------------
 * UART initialization
 * ------------------------------------------------------------ */

void uart_init(void)
{
    /*
     * Disable UART while configuring it.
     */
    UART_CR = 0UL;

    /*
     * 115200 baud @ 50 MHz.
     *
     * IBRD = 27
     * FBRD = 8
     */
    UART_IBRD = 27UL;
    UART_FBRD = 8UL;

    /*
     * FIFO enabled.
     *
     * 8 data bits
     * no parity
     * 1 stop bit
     *
     * WLEN = 3
     * FEN  = 1
     */
    UART_LCRH = 0x60UL;

    /*
     * RX interrupt level:
     *
     * Use a low threshold so that input reaches the ISR
     * without requiring a large burst.
     *
     * IFLS = 0:
     * RX FIFO interrupt at 1/8 full.
     */
    UART_IFLS = 0UL;

    /*
     * Clear any stale RX/timeout interrupt state.
     */
    UART_ICR = UART_INT_RX | UART_INT_RT;

    /*
     * Reset software ring-buffer state.
     */
    uart_rx_head = 0U;
    uart_rx_tail = 0U;

    /*
     * Enable RX and receive-timeout interrupts.
     *
     * Do this while the NVIC interrupt is still disabled.
     */
    UART_IMSC = UART_INT_RX | UART_INT_RT;

    /*
     * Enable UART:
     *
     * UARTEN = bit 8
     * TXE    = bit 9
     * RXE    = bit 0
     */
    UART_CR = 0x301UL;

    /*
     * Clear stale interrupt state once more after enabling
     * the peripheral.
     */
    UART_ICR = UART_INT_RX | UART_INT_RT;

    /*
     * Enable UART0 IRQ5 in NVIC.
     *
     * This is deliberately the final interrupt-related step.
     */
    NVIC_ISER0 = (1UL << UART0_IRQ);

    /*
     * Create UART TX mutex once.
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
     * Wait until TX FIFO has room.
     */
    while ((UART_FR & UART_FR_TXFF) != 0UL) {
        /* wait */
    }

    UART_DR = (unsigned char)c;
}


/* ------------------------------------------------------------
 * UART receive
 *
 * Hardware FIFO is NOT polled here.
 *
 * The ISR has already moved received bytes into the
 * software ring buffer.
 * ------------------------------------------------------------ */

char uart_getc(void)
{
    uint32_t tail;
    unsigned char c;

    tail = uart_rx_tail;

    /*
     * Empty.
     */
    if (tail == uart_rx_head) {
        return 0;
    }

    c = uart_rx_buffer[tail];

    /*
     * Advance consumer index after reading the byte.
     */
    uart_rx_tail =
        (tail + 1U) & UART_RX_BUFFER_MASK;

    return (char)c;
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
 * Thread-safe UART output
 * ------------------------------------------------------------ */

void uart_puts_safe(const char *s)
{
    if (s == NULL) {
        return;
    }

    /*
     * Before scheduler start, mutex cannot be used.
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
