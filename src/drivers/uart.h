#ifndef UART_H
#define UART_H

#include "FreeRTOS.h"
#include "semphr.h"

extern SemaphoreHandle_t xUARTMutex;

void uart_init(void);

void uart_putc(char c);
char uart_getc(void);

void uart_puts(const char *s);
void uart_puts_safe(const char *s);

void uart_print_hex(unsigned int val);

#endif
