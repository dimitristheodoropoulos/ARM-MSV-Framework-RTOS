/**
 * power.c — Power Management Module
 * ============================================================
 * Bare-metal ARM Cortex-M power management
 * No RTOS, no HAL library — direct register access
 *
 * Relevant for: Renesas (IoT/BLE, battery optimization)
 *
 * Architecture notes:
 *   - Sleep    : WFI instruction, SCR.SLEEPDEEP=0
 *   - DeepSleep: WFI instruction, SCR.SLEEPDEEP=1
 *   - Stop     : DeepSleep + peripheral clock gating
 *
 * In a real Renesas RA/RX design:
 *   - Replace RCGC/SCB registers with Renesas-specific
 *     SYSTEM.PRCR, SYSTEM.SBYCR, SYSTEM.MSTPCRA registers
 *   - The sleep/wake logic remains identical
 * ============================================================
 */

#include "power.h"
#include "uart.h"
#include "timer.h"

/* ── LM3S6965 System Control registers ─────────────────────── */
#define SYSCTL_BASE     0x400FE000
#define SYSCTL_RCC      (*(volatile unsigned int *)(SYSCTL_BASE + 0x060))
#define SYSCTL_RCGC1    (*(volatile unsigned int *)(SYSCTL_BASE + 0x104))
#define SYSCTL_SCGC1    (*(volatile unsigned int *)(SYSCTL_BASE + 0x114))
#define SYSCTL_DCGC1    (*(volatile unsigned int *)(SYSCTL_BASE + 0x124))

/* ── Module state ───────────────────────────────────────────── */
static volatile int  current_mode     = POWER_MODE_RUN;
static volatile unsigned int run_ticks        = 0;
static volatile unsigned int sleep_ticks      = 0;
static volatile unsigned int deep_sleep_ticks = 0;
static volatile unsigned int stop_ticks       = 0;
static volatile unsigned int last_tick        = 0;

/* ── Internal helpers ──────────────────────────────────────── */

static void print_int(unsigned int val)
{
    if (val >= 10) print_int(val / 10);
    uart_putc('0' + (val % 10));
}

/**
 * accumulate_ticks() — add elapsed time to current mode counter
 * Call before any mode transition.
 */
static void accumulate_ticks(void)
{
    unsigned int now     = get_ticks();
    unsigned int elapsed = now - last_tick;
    last_tick = now;

    switch (current_mode) {
        case POWER_MODE_RUN:        run_ticks        += elapsed; break;
        case POWER_MODE_SLEEP:      sleep_ticks      += elapsed; break;
        case POWER_MODE_DEEP_SLEEP: deep_sleep_ticks += elapsed; break;
        case POWER_MODE_STOP:       stop_ticks       += elapsed; break;
        default: break;
    }
}

/* ── Public API ─────────────────────────────────────────────── */

void power_init(void)
{
    /* Clear SLEEPDEEP bit — default to normal sleep */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    SCB_SCR &= ~SCB_SCR_SLEEPONEXIT;

    current_mode = POWER_MODE_RUN;
    last_tick    = get_ticks();

    uart_puts("[POWER] Initialized — mode: RUN\r\n");
}

/**
 * enter_sleep() — ARM Sleep mode
 *
 * SCR.SLEEPDEEP = 0
 * WFI → CPU halted, SysTick + UART + DMA still active
 * Wake: any pending or new interrupt
 *
 * Typical use (Renesas IoT):
 *   Wait for UART RX byte or timer event while saving power.
 */
void enter_sleep(void)
{
    accumulate_ticks();
    current_mode = POWER_MODE_SLEEP;

    /* Ensure SLEEPDEEP is cleared */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;

    /* Data Synchronization Barrier — flush pipeline */
    __asm volatile("dsb");
    /* Instruction Synchronization Barrier */
    __asm volatile("isb");
    /* Wait For Interrupt — CPU halts here */
    __asm volatile("wfi");

    /* Execution resumes here after wake */
    accumulate_ticks();
    current_mode = POWER_MODE_RUN;
}

/**
 * enter_deep_sleep() — ARM Deep Sleep mode
 *
 * SCR.SLEEPDEEP = 1
 * Most peripheral clocks gated by hardware.
 * On LM3S6965: Flash put into low-power mode automatically.
 * Wake: UART RX, GPIO edge, RTC, WDT
 *
 * Typical use (Renesas IoT):
 *   Sensor node waiting for data ready interrupt (1-10 sec).
 *   ~90% current reduction vs RUN mode.
 */
void enter_deep_sleep(void)
{
    accumulate_ticks();
    current_mode = POWER_MODE_DEEP_SLEEP;

    uart_puts("[POWER] Entering Deep Sleep — wake on IRQ\r\n");

    /* Set SLEEPDEEP */
    SCB_SCR |= SCB_SCR_SLEEPDEEP;

    __asm volatile("dsb");
    __asm volatile("isb");
    __asm volatile("wfi");

    /* Clear SLEEPDEEP on wake */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;

    accumulate_ticks();
    current_mode = POWER_MODE_RUN;

    uart_puts("[POWER] Woke from Deep Sleep\r\n");
}

/**
 * enter_stop_mode() — Maximum power saving
 *
 * All peripheral clocks gated via DCGC (Deep-sleep Clock Gating).
 * Only RAM contents preserved.
 * Wake: external GPIO interrupt only.
 *
 * IMPORTANT: After wake, clocks must be re-initialized.
 *            Call uart_init(), timer_init() before using them.
 *
 * Typical use (Renesas IoT):
 *   Device idle for >30 seconds, wake on button press.
 *   ~95-99% current reduction.
 */
void enter_stop_mode(void)
{
    accumulate_ticks();
    current_mode = POWER_MODE_STOP;

    uart_puts("[POWER] Entering Stop mode — gate all clocks\r\n");
    uart_puts("[POWER] Re-init required after wake\r\n");

    /* Gate UART, Timer, SSI clocks in deep-sleep */
    SYSCTL_DCGC1 = 0x00000000;

    SCB_SCR |= SCB_SCR_SLEEPDEEP;

    __asm volatile("dsb");
    __asm volatile("isb");
    __asm volatile("wfi");

    /* ── Wake from Stop ── */

    /* Restore peripheral clocks */
    SYSCTL_RCGC1 = SYSCTL_DCGC1;   /* re-enable run-mode clocks */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;

    accumulate_ticks();
    current_mode = POWER_MODE_RUN;

    uart_puts("[POWER] Woke from Stop — re-init peripherals\r\n");
}

int power_get_mode(void)
{
    return current_mode;
}

/**
 * power_print_stats() — power budget report via UART
 *
 * Shows time spent in each mode. Useful for:
 *   - Renesas: optimize duty cycle for battery life
 *   - THEON:   field diagnostics, power audit
 */
void power_print_stats(void)
{
    accumulate_ticks();

    unsigned int total = run_ticks + sleep_ticks +
                         deep_sleep_ticks + stop_ticks;
    if (total == 0) total = 1;  /* avoid division by zero */

    uart_puts("\r\n[POWER STATS]\r\n");
    uart_puts("  Total uptime   : "); print_int(total);        uart_puts(" ms\r\n");
    uart_puts("  RUN            : "); print_int(run_ticks);    uart_puts(" ms\r\n");
    uart_puts("  SLEEP          : "); print_int(sleep_ticks);  uart_puts(" ms\r\n");
    uart_puts("  DEEP SLEEP     : "); print_int(deep_sleep_ticks); uart_puts(" ms\r\n");
    uart_puts("  STOP           : "); print_int(stop_ticks);   uart_puts(" ms\r\n");

    /* Simple percentage: (val * 100) / total */
    uart_puts("  RUN %          : ");
    print_int((run_ticks * 100) / total);
    uart_puts("%\r\n");

    uart_puts("  Low-power %    : ");
    print_int(((sleep_ticks + deep_sleep_ticks + stop_ticks) * 100) / total);
    uart_puts("%\r\n\r\n");
}