#ifndef TIMER_H
#define TIMER_H

/**
 * timer.h — SysTick Timer Driver
 * ============================================================
 * ARM Cortex-M SysTick — 24-bit countdown timer
 * Provides 1ms tick resolution for all timing needs.
 *
 * Relevant for:
 *   THEON    — precise timing for sensor sampling
 *   Renesas  — duty cycle measurement for power management
 *   u-blox   — NMEA sentence timeout detection
 *   TechBiz  — HAL timer abstraction
 *
 * Vector table integration:
 *   SysTick_Handler() must be placed at vector offset 15
 *   (0x3C from vector table base). See startup.s.
 * ============================================================
 */

/* ── SysTick registers (ARM Cortex-M standard) ──────────────── */
#define SYSTICK_BASE    0xE000E010
#define SYST_CSR  (*(volatile unsigned int *)(SYSTICK_BASE + 0x00))
#define SYST_RVR  (*(volatile unsigned int *)(SYSTICK_BASE + 0x04))
#define SYST_CVR  (*(volatile unsigned int *)(SYSTICK_BASE + 0x08))
#define SYST_CALIB (*(volatile unsigned int *)(SYSTICK_BASE + 0x0C))

/* ── CSR bit fields ─────────────────────────────────────────── */
#define SYST_CSR_ENABLE     (1 << 0)  /* Counter enable         */
#define SYST_CSR_TICKINT    (1 << 1)  /* IRQ on countdown to 0  */
#define SYST_CSR_CLKSRC     (1 << 2)  /* 1=processor, 0=extref  */
#define SYST_CSR_COUNTFLAG  (1 << 16) /* Set when counted to 0  */

/* ── Clock configuration ────────────────────────────────────── */
#define SYSTICK_CLOCK_HZ    12500000UL  /* LM3S6965 QEMU: 12.5 MHz */
#define SYSTICK_TICK_HZ     1000UL      /* 1 kHz → 1 ms per tick    */
#define SYSTICK_RELOAD      (SYSTICK_CLOCK_HZ / SYSTICK_TICK_HZ - 1)

/* ── Public API ─────────────────────────────────────────────── */

/**
 * timer_init() — configure and start SysTick at 1ms intervals
 * Enables SysTick interrupt → SysTick_Handler() called every 1ms
 */
void timer_init(void);

/**
 * get_ticks() — return current tick count (milliseconds since boot)
 * Safe to call from both main context and ISR.
 */
unsigned int get_ticks(void);

/**
 * sleep_ms() — blocking delay in milliseconds
 * Uses tick comparison — safe against 32-bit overflow.
 */
void sleep_ms(unsigned int ms);

/**
 * timer_elapsed() — check if 'ms' milliseconds have passed since 'start'
 * Non-blocking — use in polling loops.
 * Example:
 *   unsigned int t = get_ticks();
 *   if (timer_elapsed(t, 500)) { ... }  // true after 500ms
 */
int timer_elapsed(unsigned int start_tick, unsigned int ms);

/**
 * SysTick_Handler() — ISR, called every 1ms by hardware
 * Declared here so startup.s can reference it by name.
 * Do NOT call directly.
 */
void SysTick_Handler(void);

#endif /* TIMER_H */