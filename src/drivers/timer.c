/**
 * timer.c — SysTick Bridge for FreeRTOS
 */

#include "timer.h"
#include "FreeRTOS.h"
#include "task.h"

/* Σημείωση: Δεν ορίζουμε SysTick_Handler εδώ. 
   Το FreeRTOS χρησιμοποιεί τον δικό του στο port.c */

void timer_init(void) {
    /* Στο FreeRTOS ο SysTick ρυθμίζεται αυτόματα κατά την 
       εκκίνηση του Scheduler (vTaskStartScheduler). */
}

unsigned int get_ticks(void) {
    /* Επιστρέφει τα milliseconds από την αρχή του OS */
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return 0;
    }
    return (unsigned int)xTaskGetTickCount();
}

/**
 * timer_elapsed() — Υπολογισμός αν πέρασε ο χρόνος ms
 * Απαραίτητο για το ESP8266 driver.
 */
int timer_elapsed(unsigned int start_tick, unsigned int ms) {
    return ((get_ticks() - start_tick) >= ms) ? 1 : 0;
}

void sleep_ms(unsigned int ms) {
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        /* Αν ο scheduler δεν τρέχει ακόμα, κάνουμε ένα απλό busy wait */
        for (volatile int i = 0; i < ms * 2000; i++);
    } else {
        /* Αν τρέχει το OS, παραχωρούμε τον επεξεργαστή */
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}