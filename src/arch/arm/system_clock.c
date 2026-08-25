#include "system_clock.h"
#include <stdint.h>

/*
 * LM3S6965 / QEMU Stellaris System Control
 */
#define SYSCTL_BASE       0x400FE000UL

#define SYSCTL_RIS        (*(volatile uint32_t *)(SYSCTL_BASE + 0x050UL))
#define SYSCTL_RCC        (*(volatile uint32_t *)(SYSCTL_BASE + 0x060UL))
#define SYSCTL_RCC2       (*(volatile uint32_t *)(SYSCTL_BASE + 0x070UL))


/* ------------------------------------------------------------
 * RCC
 * ------------------------------------------------------------ */

#define RCC_XTAL_MASK     0x000007C0UL
#define RCC_XTAL_8MHZ     0x00000540UL


/* ------------------------------------------------------------
 * RCC2
 * ------------------------------------------------------------ */

#define RCC2_USERCC2      0x80000000UL
#define RCC2_SYSDIV2_MASK 0x1F800000UL
#define RCC2_SYSDIV2_4    (3UL << 23)

#define RCC2_BYPASS2      0x00000800UL
#define RCC2_PWRDN2       0x00002000UL


/* ------------------------------------------------------------
 * RIS
 * ------------------------------------------------------------ */

/*
 * PLLLRIS = PLL Lock Raw Interrupt Status
 *
 * QEMU Stellaris implements bit 6 and sets it when the PLL
 * transitions from powered-down to enabled.
 */
#define RIS_PLLLRIS       0x00000040UL


/* ------------------------------------------------------------
 * PLL lock timeout
 * ------------------------------------------------------------ */

/*
 * The timeout is only a safety guard.
 *
 * QEMU does not emulate a physical PLL lock delay.
 */
#define PLL_LOCK_TIMEOUT  1000000UL


/* ------------------------------------------------------------
 * system_clock_init
 * ------------------------------------------------------------ */

void system_clock_init(void)
{
    uint32_t rcc;
    uint32_t rcc2;
    uint32_t timeout;


    /*
     * --------------------------------------------------------
     * 1. Enable RCC2.
     * --------------------------------------------------------
     *
     * This selects the extended RCC2 clock configuration.
     */
    rcc2 = SYSCTL_RCC2;

    rcc2 |= RCC2_USERCC2;

    /*
     * Keep PLL bypassed while changing clock configuration.
     */
    rcc2 |= RCC2_BYPASS2;

    SYSCTL_RCC2 = rcc2;


    /*
     * --------------------------------------------------------
     * 2. Select the 8 MHz crystal.
     * --------------------------------------------------------
     *
     * LM3S6965 uses the 8 MHz crystal selection.
     */
    rcc = SYSCTL_RCC;

    rcc &= ~RCC_XTAL_MASK;
    rcc |= RCC_XTAL_8MHZ;

    SYSCTL_RCC = rcc;


    /*
     * --------------------------------------------------------
     * 3. Select the main oscillator.
     * --------------------------------------------------------
     *
     * OSCSRC2 = 0
     *
     * This is the main oscillator source.
     */
    rcc2 = SYSCTL_RCC2;

    rcc2 &= ~0x00000070UL;


    /*
     * --------------------------------------------------------
     * 4. Configure PLL/system divider.
     * --------------------------------------------------------
     *
     * QEMU Stellaris:
     *
     *     PLL input = 200 MHz
     *
     *     SYSDIV2 = 3
     *
     *     divisor = SYSDIV2 + 1
     *             = 4
     *
     *     SYSCLK = 200 MHz / 4
     *            = 50 MHz
     */
    rcc2 &= ~RCC2_SYSDIV2_MASK;
    rcc2 |= RCC2_SYSDIV2_4;


    /*
     * --------------------------------------------------------
     * 5. Enable PLL.
     * --------------------------------------------------------
     */
    rcc2 &= ~RCC2_PWRDN2;

    /*
     * Apply the configuration while PLL remains bypassed.
     */
    SYSCTL_RCC2 = rcc2;


    /*
     * --------------------------------------------------------
     * 6. Wait for PLL lock, but never forever.
     * --------------------------------------------------------
     */
    timeout = PLL_LOCK_TIMEOUT;

    while (((SYSCTL_RIS & RIS_PLLLRIS) == 0U) &&
           (timeout != 0U)) {

        timeout--;
    }


    /*
     * --------------------------------------------------------
     * 7. Switch from oscillator bypass to PLL.
     * --------------------------------------------------------
     *
     * We intentionally do not hang the entire firmware if the
     * emulated PLL status is unavailable.
     *
     * The QEMU model propagates the system clock from RCC/RCC2
     * configuration itself.
     */
    rcc2 = SYSCTL_RCC2;

    rcc2 &= ~RCC2_BYPASS2;

    SYSCTL_RCC2 = rcc2;
}