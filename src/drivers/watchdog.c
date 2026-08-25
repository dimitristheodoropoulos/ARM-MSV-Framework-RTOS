#include "FreeRTOS.h"
#include "task.h"
#include "system_clock.h"

#include <stdint.h>
#include <limits.h>


/*
 * LM3S6965 Watchdog Timer
 */
#define WDT_BASE    0x40000000UL

#define WDT_LOAD \
    (*(volatile uint32_t *)(WDT_BASE + 0x000UL))

#define WDT_CTL \
    (*(volatile uint32_t *)(WDT_BASE + 0x008UL))

#define WDT_ICR \
    (*(volatile uint32_t *)(WDT_BASE + 0x00CUL))

#define WDT_LOCK \
    (*(volatile uint32_t *)(WDT_BASE + 0xC00UL))


#define WDT_LOCK_KEY    0x1ACCE551UL


void watchdog_init(unsigned int timeout_ms)
{
    uint64_t cycles;

    /*
     * Convert milliseconds to clock cycles.
     *
     *     cycles =
     *         timeout_ms * SYSTEM_CLOCK_HZ / 1000
     *
     * SYSTEM_CLOCK_HZ = 50 MHz.
     */
    cycles =
        ((uint64_t)timeout_ms * SYSTEM_CLOCK_HZ) / 1000ULL;

    /*
     * Prevent wrap-around if an invalidly large timeout
     * is supplied.
     */
    if (cycles > UINT32_MAX) {
        cycles = UINT32_MAX;
    }

    WDT_LOCK = WDT_LOCK_KEY;

    WDT_LOAD = (uint32_t)cycles;

    /*
     * Enable:
     *
     * bit 0 = watchdog enable
     * bit 1 = reset enable
     */
    WDT_CTL = 0x03UL;

    WDT_LOCK = 0UL;
}


void watchdog_feed(void)
{
    /*
     * Clear/reload watchdog counter.
     */
    WDT_ICR = 0xFFFFFFFFUL;
}
