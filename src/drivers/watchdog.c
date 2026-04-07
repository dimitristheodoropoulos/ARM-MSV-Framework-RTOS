#include "FreeRTOS.h"
#include "task.h"

#define WDT_BASE    0x40000000
#define WDT_LOAD    (*(volatile unsigned int *)(WDT_BASE + 0x000))
#define WDT_CTL     (*(volatile unsigned int *)(WDT_BASE + 0x008))
#define WDT_ICR     (*(volatile unsigned int *)(WDT_BASE + 0x00C))
#define WDT_LOCK    (*(volatile unsigned int *)(WDT_BASE + 0xC00))

void watchdog_init(unsigned int timeout_ms) {
    // 16MHz clock -> 1ms = 16,000 cycles
    unsigned int load_val = timeout_ms * 16000;

    WDT_LOCK = 0x1ACCE551; // Ξεκλείδωμα registers
    WDT_LOAD = load_val;
    WDT_CTL  = 0x03;       // Ενεργοποίηση Reset + Ενεργοποίηση WDT
    WDT_LOCK = 0;          // Κλείδωμα
}

void watchdog_feed(void) {
    WDT_ICR = 0xFFFFFFFF;  // "Kick the dog" - Επαναφορά του μετρητή
}