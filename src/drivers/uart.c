#include "uart.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define UART0_BASE 0x4000C000
#define UART_DR    (*(volatile unsigned char *)(UART0_BASE + 0x00))
#define UART_FR    (*(volatile unsigned int  *)(UART0_BASE + 0x18))
#define UART_IBRD  (*(volatile unsigned int  *)(UART0_BASE + 0x24))
#define UART_FBRD  (*(volatile unsigned int  *)(UART0_BASE + 0x28))
#define UART_LCRH  (*(volatile unsigned int  *)(UART0_BASE + 0x2C))
#define UART_CR    (*(volatile unsigned int  *)(UART0_BASE + 0x30))

SemaphoreHandle_t xUARTMutex = NULL;

void uart_init(void) {
    UART_CR = 0;
    UART_IBRD = 8;
    UART_FBRD = 44;
    UART_LCRH = 0x60;
    UART_CR = 0x301;
    
    if (xUARTMutex == NULL) {
        xUARTMutex = xSemaphoreCreateMutex();
    }
}

void uart_putc(char c) {
    while (UART_FR & (1 << 5));
    UART_DR = c;
}

// SENIOR FIX: Non-blocking getc
char uart_getc(void) {
    // Αν το bit 4 (RXFE - Receive FIFO Empty) είναι 1, δεν υπάρχει δεδομένο
    if (UART_FR & (1 << 4)) {
        return 0; // Επιστροφή 0 αντί για πάγωμα
    }
    return UART_DR;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

void uart_puts_safe(const char *s) {
    if (xUARTMutex != NULL && xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        if (xSemaphoreTake(xUARTMutex, portMAX_DELAY) == pdTRUE) {
            uart_puts(s);
            xSemaphoreGive(xUARTMutex);
        }
    } else {
        uart_puts(s);
    }
}

void uart_print_hex(unsigned int val) {
    const char *hex = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4)
        uart_putc(hex[(val >> i) & 0xF]);
}