#ifndef TIMER_H
#define TIMER_H

/**
 * timer.h — FreeRTOS timing abstraction
 *
 * FreeRTOS:
 *
 *     configTICK_RATE_HZ = 100 Hz
 *     1 OS tick          = 10 ms
 *
 * The timer API exposed here uses milliseconds.
 *
 * SysTick itself is owned and configured by the
 * FreeRTOS Cortex-M3 port.
 */

void timer_init(void);

unsigned int get_ticks(void);

void sleep_ms(unsigned int ms);

int timer_elapsed(
    unsigned int start_tick,
    unsigned int ms
);

#endif /* TIMER_H */
