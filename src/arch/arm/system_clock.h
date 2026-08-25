#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

/*
 * LM3S6965 / QEMU system clock
 *
 * QEMU Stellaris clock model:
 *
 *     PLL/system input = 200 MHz
 *
 *     SYSDIV2 = 3
 *
 *     divisor = SYSDIV2 + 1 = 4
 *
 *     SYSCLK = 200 MHz / 4
 *            = 50 MHz
 *
 * This is the single source of truth for firmware timing.
 */

#define SYSTEM_CLOCK_HZ    50000000UL

void system_clock_init(void);

#endif /* SYSTEM_CLOCK_H */