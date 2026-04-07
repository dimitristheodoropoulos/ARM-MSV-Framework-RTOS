/**
 * i2c.c — I2C Master Driver
 * ============================================================
 * LM3S6965 / STM32F4 compatible I2C master driver
 * Uses STM32F4 I2C register map (I2C1 at 0x40005400)
 *
 * Register map used (STM32F4 I2C):
 *   CR1   — Control Register 1 (START, STOP, ACK, PE)
 *   CR2   — Control Register 2 (FREQ, ITEVTEN, ITERREN)
 *   DR    — Data Register (TX and RX)
 *   SR1   — Status Register 1 (SB, ADDR, BTF, RXNE, TXE)
 *   SR2   — Status Register 2 (MSL, BUSY, TRA)
 *   CCR   — Clock Control Register
 *   TRISE — Rise Time Register
 *
 * Relevant for: THEON (sensor bus), Renesas (IoT), TechBiz
 * ============================================================
 */

#include "i2c.h"
#include "uart.h"
#include "timer.h"

/* ── Register definitions ───────────────────────────────────── */
#define I2C1_BASE   0x40005400UL

#define I2C_CR1     (*(volatile unsigned int *)(I2C1_BASE + 0x00))
#define I2C_CR2     (*(volatile unsigned int *)(I2C1_BASE + 0x04))
#define I2C_DR      (*(volatile unsigned int *)(I2C1_BASE + 0x10))
#define I2C_SR1     (*(volatile unsigned int *)(I2C1_BASE + 0x14))
#define I2C_SR2     (*(volatile unsigned int *)(I2C1_BASE + 0x18))
#define I2C_CCR     (*(volatile unsigned int *)(I2C1_BASE + 0x1C))
#define I2C_TRISE   (*(volatile unsigned int *)(I2C1_BASE + 0x20))

/* ── CR1 bits ───────────────────────────────────────────────── */
#define I2C_CR1_PE      (1 << 0)   /* Peripheral enable         */
#define I2C_CR1_START   (1 << 8)   /* Generate START condition  */
#define I2C_CR1_STOP    (1 << 9)   /* Generate STOP condition   */
#define I2C_CR1_ACK     (1 << 10)  /* ACK enable                */
#define I2C_CR1_SWRST   (1 << 15)  /* Software reset            */

/* ── SR1 bits ───────────────────────────────────────────────── */
#define I2C_SR1_SB      (1 << 0)   /* START generated           */
#define I2C_SR1_ADDR    (1 << 1)   /* Address sent / matched    */
#define I2C_SR1_BTF     (1 << 2)   /* Byte transfer finished    */
#define I2C_SR1_RXNE    (1 << 6)   /* RX register not empty     */
#define I2C_SR1_TXE     (1 << 7)   /* TX register empty         */
#define I2C_SR1_BERR    (1 << 8)   /* Bus error                 */
#define I2C_SR1_ARLO    (1 << 9)   /* Arbitration lost          */
#define I2C_SR1_AF      (1 << 10)  /* Acknowledge failure (NACK)*/

/* ── Timeout ────────────────────────────────────────────────── */
#define I2C_TIMEOUT_MS  10         /* 10ms max wait per step    */

/* ── APB1 clock for CCR calculation ────────────────────────── */
#define APB1_CLOCK_HZ   42000000UL /* STM32F4 APB1 @ 42 MHz    */
#define APB1_CLOCK_MHZ  42         /* for TRISE register        */

/* ── Internal helpers ──────────────────────────────────────── */

/**
 * wait_flag() — poll SR1 bit with timeout
 * Returns I2C_OK when flag set, I2C_ERR_TOUT on timeout.
 */
static int wait_flag(unsigned int flag)
{
    unsigned int start = get_ticks();
    while (!(I2C_SR1 & flag)) {
        if ((get_ticks() - start) >= I2C_TIMEOUT_MS)
            return I2C_ERR_TOUT;
        /* Check for error flags */
        if (I2C_SR1 & I2C_SR1_BERR) { I2C_SR1 &= ~I2C_SR1_BERR; return I2C_ERR_BUS;  }
        if (I2C_SR1 & I2C_SR1_ARLO) { I2C_SR1 &= ~I2C_SR1_ARLO; return I2C_ERR_ARB;  }
        if (I2C_SR1 & I2C_SR1_AF)   { I2C_SR1 &= ~I2C_SR1_AF;   return I2C_ERR_NACK; }
    }
    return I2C_OK;
}

/**
 * send_start() — generate START and send address
 * @addr_rw: (7-bit addr << 1) | R/W bit (0=write, 1=read)
 */
static int send_start(unsigned char addr_rw)
{
    int err;

    /* Generate START */
    I2C_CR1 |= I2C_CR1_START;
    err = wait_flag(I2C_SR1_SB);
    if (err) return err;

    /* Send address + R/W bit */
    I2C_DR = addr_rw;
    err = wait_flag(I2C_SR1_ADDR);
    if (err) return err;

    /* Clear ADDR by reading SR1 then SR2 */
    (void)I2C_SR1;
    (void)I2C_SR2;

    return I2C_OK;
}

/**
 * send_stop() — generate STOP condition
 */
static void send_stop(void)
{
    I2C_CR1 |= I2C_CR1_STOP;
    /* Brief wait for STOP to complete */
    unsigned int start = get_ticks();
    while ((I2C_SR2 & (1 << 1)) && (get_ticks() - start) < I2C_TIMEOUT_MS);
}

/* ── Public API ─────────────────────────────────────────────── */

void i2c_init(unsigned int speed)
{
    /* Software reset to clear any stuck state */
    I2C_CR1 |=  I2C_CR1_SWRST;
    I2C_CR1 &= ~I2C_CR1_SWRST;

    /* Set APB1 frequency in CR2 (bits [5:0]) */
    I2C_CR2 = APB1_CLOCK_MHZ & 0x3F;

    /* Configure CCR (Clock Control Register) */
    if (speed == I2C_SPEED_FAST) {
        /* Fast mode: CCR = APB1 / (25 * speed) */
        unsigned int ccr = APB1_CLOCK_HZ / (25 * I2C_SPEED_FAST);
        I2C_CCR = (1 << 15) | (1 << 14) | (ccr & 0xFFF); /* FM, DUTY=1 */
        /* TRISE = (APB1_MHz * 0.3us) + 1 */
        I2C_TRISE = (APB1_CLOCK_MHZ * 3) / 10 + 1;
    } else {
        /* Standard mode: CCR = APB1 / (2 * 100kHz) */
        unsigned int ccr = APB1_CLOCK_HZ / (2 * I2C_SPEED_STANDARD);
        I2C_CCR = ccr & 0xFFF;
        /* TRISE = APB1_MHz + 1 (for 1us rise time @ 100kHz) */
        I2C_TRISE = APB1_CLOCK_MHZ + 1;
    }

    /* Enable peripheral */
    I2C_CR1 |= I2C_CR1_PE;
    I2C_CR1 |= I2C_CR1_ACK;
}

int i2c_write_byte(unsigned char dev_addr,
                   unsigned char reg,
                   unsigned char data)
{
    int err;

    /* START + address (write) */
    err = send_start((dev_addr << 1) | 0);
    if (err) { send_stop(); return err; }

    /* Send register address */
    err = wait_flag(I2C_SR1_TXE);
    if (err) { send_stop(); return err; }
    I2C_DR = reg;

    /* Send data byte */
    err = wait_flag(I2C_SR1_TXE);
    if (err) { send_stop(); return err; }
    I2C_DR = data;

    /* Wait for transfer complete */
    err = wait_flag(I2C_SR1_BTF);
    send_stop();
    return err;
}

int i2c_read_byte(unsigned char  dev_addr,
                  unsigned char  reg,
                  unsigned char *data)
{
    int err;

    /* START + address (write) — send register pointer */
    err = send_start((dev_addr << 1) | 0);
    if (err) { send_stop(); return err; }

    err = wait_flag(I2C_SR1_TXE);
    if (err) { send_stop(); return err; }
    I2C_DR = reg;

    err = wait_flag(I2C_SR1_BTF);
    if (err) { send_stop(); return err; }

    /* Repeated START + address (read) */
    err = send_start((dev_addr << 1) | 1);
    if (err) { send_stop(); return err; }

    /* Disable ACK before reading last byte */
    I2C_CR1 &= ~I2C_CR1_ACK;
    send_stop();

    err = wait_flag(I2C_SR1_RXNE);
    if (err) return err;

    *data = (unsigned char)I2C_DR;

    /* Re-enable ACK for future transfers */
    I2C_CR1 |= I2C_CR1_ACK;
    return I2C_OK;
}

int i2c_write_burst(unsigned char        dev_addr,
                    unsigned char        reg,
                    const unsigned char *buf,
                    unsigned int         len)
{
    int err;

    err = send_start((dev_addr << 1) | 0);
    if (err) { send_stop(); return err; }

    /* Send register address */
    err = wait_flag(I2C_SR1_TXE);
    if (err) { send_stop(); return err; }
    I2C_DR = reg;

    /* Send data bytes */
    for (unsigned int i = 0; i < len; i++) {
        err = wait_flag(I2C_SR1_TXE);
        if (err) { send_stop(); return err; }
        I2C_DR = buf[i];
    }

    err = wait_flag(I2C_SR1_BTF);
    send_stop();
    return err;
}

int i2c_read_burst(unsigned char  dev_addr,
                   unsigned char  reg,
                   unsigned char *buf,
                   unsigned int   len)
{
    int err;

    /* Write phase — send register pointer */
    err = send_start((dev_addr << 1) | 0);
    if (err) { send_stop(); return err; }

    err = wait_flag(I2C_SR1_TXE);
    if (err) { send_stop(); return err; }
    I2C_DR = reg;

    err = wait_flag(I2C_SR1_BTF);
    if (err) { send_stop(); return err; }

    /* Read phase */
    err = send_start((dev_addr << 1) | 1);
    if (err) { send_stop(); return err; }

    for (unsigned int i = 0; i < len; i++) {
        /* Disable ACK before last byte */
        if (i == len - 1) {
            I2C_CR1 &= ~I2C_CR1_ACK;
            send_stop();
        }
        err = wait_flag(I2C_SR1_RXNE);
        if (err) return err;
        buf[i] = (unsigned char)I2C_DR;
    }

    I2C_CR1 |= I2C_CR1_ACK;
    return I2C_OK;
}

void i2c_scan(void)
{
    uart_puts("\r\n[I2C SCAN] Scanning 0x00-0x7F...\r\n");
    uart_puts("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");

    int found = 0;
    for (int row = 0; row < 8; row++) {
        uart_print_hex(row << 4);
        uart_puts(": ");
        for (int col = 0; col < 16; col++) {
            unsigned char addr = (unsigned char)((row << 4) | col);
            /* Skip reserved addresses 0x00-0x07 and 0x78-0x7F */
            if (addr < 0x08 || addr > 0x77) {
                uart_puts("   ");
                continue;
            }
            /* Probe: send START + addr + STOP, check for ACK */
            int err = send_start((addr << 1) | 0);
            send_stop();
            if (err == I2C_OK) {
                uart_print_hex(addr);
                uart_putc(' ');
                found++;
            } else {
                uart_puts("-- ");
            }
        }
        uart_puts("\r\n");
    }
    uart_puts("[I2C SCAN] Found ");
    uart_putc('0' + found);
    uart_puts(" device(s)\r\n\r\n");
}

const char *i2c_error_str(int err)
{
    switch (err) {
        case I2C_OK:        return "OK";
        case I2C_ERR_NACK:  return "NACK (device not responding)";
        case I2C_ERR_ARB:   return "Arbitration lost";
        case I2C_ERR_TOUT:  return "Timeout";
        case I2C_ERR_BUS:   return "Bus error (SDA/SCL stuck)";
        default:            return "Unknown error";
    }
}