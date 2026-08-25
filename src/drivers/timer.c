/**
 * timer.c — FreeRTOS timing bridge
 */

#include "timer.h"
#include "FreeRTOS.h"
#include "task.h"

/*
 * SysTick is configured by the FreeRTOS Cortex-M3 port.
 *
 * We deliberately do not define SysTick_Handler here.
 */

void timer_init(void)
{
    /*
     * FreeRTOS configures SysTick during vTaskStartScheduler().
     */
}

unsigned int get_ticks(void)
{
    TickType_t ticks;

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return 0U;
    }

    ticks = xTaskGetTickCount();

    /*
     * Convert FreeRTOS ticks to milliseconds.
     *
     * At 100 Hz:
     *
     *   1 tick = 10 ms
     */
    return (unsigned int)(
        ((uint32_t)ticks * 1000UL) / (uint32_t)configTICK_RATE_HZ
    );
}

int timer_elapsed(unsigned int start_tick, unsigned int ms)
{
    return ((get_ticks() - start_tick) >= ms) ? 1 : 0;
}

void sleep_ms(unsigned int ms)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {

        /*
         * Scheduler has not started yet.
         *
         * This fallback is only for early boot code and is not
         * cycle-accurate. Runtime task delays use FreeRTOS below.
         */
        for (volatile unsigned int i = 0;
             i < (ms * 1000UL);
             ++i) {
            __asm volatile ("nop");
        }

    } else {

        /*
         * FreeRTOS performs the actual scheduler-aware delay.
         */
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}
