/**
 * spi.c — SPI Master Driver
 * ============================================================
 * STM32F4 SPI1 register map (base: 0x40013000)
 * Compatible with LM3S6965 SSI peripheral (base: 0x40008000)
 *
 * Relevant for:
 *   TechBiz — FPGA SPI bridge (spi_slave_bfm.v co-simulation)
 *   THEON   — SPI sensor interface (IMU, ADC)
 *   Renesas — SPI flash, display, sensor interfaces
 * ============================================================
 */

#include "spi.h"
#include "uart.h"
#include "timer.h"

/* ── STM32F4 SPI1 registers ─────────────────────────────────── */
#define SPI1_BASE   0x40013000UL

#define SPI_CR1     (*(volatile unsigned int *)(SPI1_BASE + 0x00))
#define SPI_CR2     (*(volatile unsigned int *)(SPI1_BASE + 0x04))
#define SPI_SR      (*(volatile unsigned int *)(SPI1_BASE + 0x08))
#define SPI_DR      (*(volatile unsigned int *)(SPI1_BASE + 0x0C))
#define SPI_CRCPR   (*(volatile unsigned int *)(SPI1_BASE + 0x10))

/* ── CR1 bits ───────────────────────────────────────────────── */
#define SPI_CR1_CPHA    (1 << 0)    /* Clock phase               */
#define SPI_CR1_CPOL    (1 << 1)    /* Clock polarity            */
#define SPI_CR1_MSTR    (1 << 2)    /* Master mode               */
#define SPI_CR1_BR_MASK (7 << 3)    /* Baud rate [5:3]           */
#define SPI_CR1_SPE     (1 << 6)    /* SPI enable                */
#define SPI_CR1_LSBFIRST (1 << 7)    /* LSB transmitted first     */
#define SPI_CR1_SSI     (1 << 8)    /* Internal slave select     */
#define SPI_CR1_SSM     (1 << 9)    /* Software slave management */
#define SPI_CR1_DFF     (1 << 11)   /* Data frame format (16-bit)*/

/* ── SR bits ────────────────────────────────────────────────── */
#define SPI_SR_RXNE     (1 << 0)    /* RX buffer not empty       */
#define SPI_SR_TXE      (1 << 1)    /* TX buffer empty           */
#define SPI_SR_OVR      (1 << 6)    /* Overrun flag              */
#define SPI_SR_BSY      (1 << 7)    /* SPI busy                  */

/* ── GPIO for CS (GPIOA Pin 4 — SPI1_NSS) ───────────────────── */
#define GPIOA_BASE  0x40020000UL
#define GPIOA_ODR   (*(volatile unsigned int *)(GPIOA_BASE + 0x14))
#define SPI_CS_PIN  (1 << 4)

/* ── Timeout ────────────────────────────────────────────────── */
#define SPI_TIMEOUT_MS  5

/* ── Baud rate divider lookup ───────────────────────────────── */
/* BR[2:0] in CR1: 000=div2, 001=div4, ..., 111=div256 */
static unsigned int clk_div_to_br(unsigned int div)
{
    switch (div) {
        case   2: return 0;
        case   4: return 1;
        case   8: return 2;
        case  16: return 3;
        case  32: return 4;
        case  64: return 5;
        case 128: return 6;
        default:  return 7; /* div256 */
    }
}

/* ── Internal helpers ──────────────────────────────────────── */

static int wait_txe(void)
{
    unsigned int start = get_ticks();
    while (!(SPI_SR & SPI_SR_TXE)) {
        if ((get_ticks() - start) >= SPI_TIMEOUT_MS) return SPI_ERR_TOUT;
    }
    return SPI_OK;
}

static int wait_rxne(void)
{
    unsigned int start = get_ticks();
    while (!(SPI_SR & SPI_SR_RXNE)) {
        if ((get_ticks() - start) >= SPI_TIMEOUT_MS) return SPI_ERR_TOUT;
        if (SPI_SR & SPI_SR_OVR) return SPI_ERR_OVR;
    }
    return SPI_OK;
}

static void wait_not_busy(void)
{
    unsigned int start = get_ticks();
    while ((SPI_SR & SPI_SR_BSY) && (get_ticks() - start) < SPI_TIMEOUT_MS);
}

/* ── Public API ─────────────────────────────────────────────── */

void spi_init(int mode, int frame_bits, unsigned int clk_div)
{
    /* Disable SPI before configuration */
    SPI_CR1 &= ~SPI_CR1_SPE;

    /* Build CR1 value */
    unsigned int cr1 = 0;

    /* Clock polarity and phase from mode */
    if (mode == SPI_MODE_1 || mode == SPI_MODE_3) cr1 |= SPI_CR1_CPHA;
    if (mode == SPI_MODE_2 || mode == SPI_MODE_3) cr1 |= SPI_CR1_CPOL;

    /* Master mode */
    cr1 |= SPI_CR1_MSTR;

    /* Baud rate */
    cr1 |= (clk_div_to_br(clk_div) << 3) & SPI_CR1_BR_MASK;

    /* Software NSS management — CS controlled manually */
    cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;

    /* 16-bit frame if requested */
    if (frame_bits == SPI_FRAME_16BIT) cr1 |= SPI_CR1_DFF;

    SPI_CR1 = cr1;

    /* Enable SPI */
    SPI_CR1 |= SPI_CR1_SPE;

    /* CS high (inactive) at startup */
    spi_cs_high();
}

void spi_cs_low(void)
{
    GPIOA_ODR &= ~SPI_CS_PIN;
}

void spi_cs_high(void)
{
    GPIOA_ODR |= SPI_CS_PIN;
}

unsigned char spi_transfer(unsigned char tx)
{
    if (wait_txe() != SPI_OK) return 0xFF;
    SPI_DR = tx;
    if (wait_rxne() != SPI_OK) return 0xFF;
    return (unsigned char)SPI_DR;
}

unsigned short spi_transfer16(unsigned short tx)
{
    if (wait_txe() != SPI_OK) return 0xFFFF;
    SPI_DR = tx;
    if (wait_rxne() != SPI_OK) return 0xFFFF;
    return (unsigned short)SPI_DR;
}

int spi_write_buf(const unsigned char *buf, unsigned int len)
{
    for (unsigned int i = 0; i < len; i++) {
        int err = wait_txe();
        if (err) return err;
        SPI_DR = buf[i];
        err = wait_rxne();
        if (err) return err;
        (void)SPI_DR;   /* discard received byte */
    }
    wait_not_busy();
    return SPI_OK;
}

int spi_read_buf(unsigned char *buf, unsigned int len)
{
    for (unsigned int i = 0; i < len; i++) {
        int err = wait_txe();
        if (err) return err;
        SPI_DR = 0xFF;  /* send dummy byte to generate clock */
        err = wait_rxne();
        if (err) return err;
        buf[i] = (unsigned char)SPI_DR;
    }
    wait_not_busy();
    return SPI_OK;
}

/**
 * spi_loopback_test() — validate SPI hardware with known pattern
 *
 * Sends 8 test bytes and verifies each is echoed correctly.
 * Requires MISO connected to MOSI (loopback jumper).
 *
 * Used by:
 *   - shell.c  "spi" command
 *   - diagnostics.c POST
 *   - TechBiz FPGA SPI bridge verification
 */
int spi_loopback_test(void)
{
    static const unsigned char patterns[] = {
        0xA5, 0x5A, 0xFF, 0x00, 0x12, 0x34, 0xAB, 0xCD
    };
    int errors = 0;

    uart_puts("\r\n[SPI] Loopback test (8 bytes)...\r\n");

    spi_cs_low();

    for (unsigned int i = 0; i < sizeof(patterns); i++) {
        unsigned char tx  = patterns[i];
        unsigned char rx  = spi_transfer(tx);
        int           ok  = (rx == tx);   /* loopback: RX must equal TX */

        uart_puts("  [");
        uart_putc('0' + i);
        uart_puts("] TX=");
        uart_print_hex(tx);
        uart_puts(" RX=");
        uart_print_hex(rx);
        uart_puts(ok ? " PASS\r\n" : " FAIL\r\n");

        if (!ok) errors++;
    }

    spi_cs_high();

    uart_puts("[SPI] Result: ");
    uart_putc('0' + (sizeof(patterns) - errors));
    uart_puts("/");
    uart_putc('0' + sizeof(patterns));
    uart_puts(errors ? " FAIL\r\n\r\n" : " PASS\r\n\r\n");

    return errors;
}

const char *spi_error_str(int err)
{
    switch (err) {
        case SPI_OK:        return "OK";
        case SPI_ERR_TOUT:  return "Timeout";
        case SPI_ERR_OVR:   return "Overrun";
        default:            return "Unknown";
    }
}