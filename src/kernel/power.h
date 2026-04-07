#ifndef POWER_H
#define POWER_H

/**
 * power.h — Power Management Module
 * ============================================================
 * Relevant for: Renesas (IoT, battery-powered devices)
 *
 * Sleep modes (ARM Cortex-M):
 *
 *   Sleep      — CPU halted, peripherals running
 *                Wake: any interrupt
 *                Current: ~50% reduction
 *
 *   Deep Sleep  — CPU + most peripherals halted
 *                Wake: specific interrupts (UART, GPIO)
 *                Current: ~90% reduction
 *
 *   Stop        — All clocks gated, RAM retained
 *                Wake: external interrupt / RTC
 *                Current: ~95% reduction
 *
 * Usage:
 *   enter_sleep()       — lightweight, wake on any IRQ
 *   enter_deep_sleep()  — aggressive, wake on UART/GPIO
 *   enter_stop_mode()   — maximum saving, RAM retained
 *   power_get_mode()    — query current power state
 *   power_print_stats() — print power mode history via UART
 * ============================================================
 */

/* ── Power mode identifiers ─────────────────────────────────── */
#define POWER_MODE_RUN        0   /* Normal operation          */
#define POWER_MODE_SLEEP      1   /* WFI — CPU halted          */
#define POWER_MODE_DEEP_SLEEP 2   /* SLEEPDEEP — clocks gated  */
#define POWER_MODE_STOP       3   /* All clocks off, RAM on    */

/* ── SCB registers ──────────────────────────────────────────── */
#define SCB_SCR     (*(volatile unsigned int *)0xE000ED10)
#define SCB_SCR_SLEEPDEEP   (1 << 2)
#define SCB_SCR_SLEEPONEXIT (1 << 1)

/* ── Public API ─────────────────────────────────────────────── */

/**
 * power_init() — initialize power management module
 * Must be called once during system init.
 */
void power_init(void);

/**
 * enter_sleep() — Sleep mode (WFI)
 * CPU halted, SysTick + UART still running.
 * Wake: any enabled interrupt.
 */
void enter_sleep(void);

/**
 * enter_deep_sleep() — Deep Sleep mode (SLEEPDEEP + WFI)
 * Most clocks gated. UART RX and GPIO can wake system.
 * Wake: UART RX interrupt or GPIO edge.
 */
void enter_deep_sleep(void);

/**
 * enter_stop_mode() — Stop mode (maximum power saving)
 * All clocks off, only RAM retained.
 * Wake: external interrupt or RTC alarm.
 * Note: re-init required after wake (clocks must be restored).
 */
void enter_stop_mode(void);

/**
 * power_get_mode() — returns current POWER_MODE_* constant
 */
int power_get_mode(void);

/**
 * power_print_stats() — print power mode statistics via UART
 * Shows time spent in each mode (based on SysTick).
 */
void power_print_stats(void);

#endif /* POWER_H */