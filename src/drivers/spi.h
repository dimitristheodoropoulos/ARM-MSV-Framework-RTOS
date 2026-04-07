#ifndef SPI_H
#define SPI_H

/**
 * spi.h — SPI Master Driver
 * ============================================================
 * ARM Cortex-M bare-metal SPI master (SSI/SPI peripheral)
 * Direct register access — no HAL, no RTOS
 *
 * Relevant for:
 *   TechBiz  — FPGA SPI bridge co-simulation
 *   THEON    — SPI sensors (IMU, ADC, flash memory)
 *   Renesas  — SPI peripheral interface
 *
 * Features:
 *   - Configurable clock polarity (CPOL) and phase (CPHA)
 *   - SPI modes 0-3
 *   - 8-bit and 16-bit frame support
 *   - Software CS (chip select) control
 *   - DMA-ready API (burst transfer)
 *   - Timeout-based error detection
 *
 * SPI Modes:
 *   Mode 0: CPOL=0, CPHA=0 — idle low,  sample on rising
 *   Mode 1: CPOL=0, CPHA=1 — idle low,  sample on falling
 *   Mode 2: CPOL=1, CPHA=0 — idle high, sample on falling
 *   Mode 3: CPOL=1, CPHA=1 — idle high, sample on rising
 * ============================================================
 */

/* ── SPI modes ──────────────────────────────────────────────── */
#define SPI_MODE_0   0   /* CPOL=0, CPHA=0 */
#define SPI_MODE_1   1   /* CPOL=0, CPHA=1 */
#define SPI_MODE_2   2   /* CPOL=1, CPHA=0 */
#define SPI_MODE_3   3   /* CPOL=1, CPHA=1 */

/* ── Frame sizes ────────────────────────────────────────────── */
#define SPI_FRAME_8BIT   8
#define SPI_FRAME_16BIT  16

/* ── Error codes ────────────────────────────────────────────── */
#define SPI_OK          0
#define SPI_ERR_TOUT   -1
#define SPI_ERR_OVR    -2   /* Overrun error */

/* ── Public API ─────────────────────────────────────────────── */

/**
 * spi_init() — initialize SPI peripheral
 * @mode:      SPI_MODE_0 .. SPI_MODE_3
 * @frame_bits:SPI_FRAME_8BIT or SPI_FRAME_16BIT
 * @clk_div:   clock divider (2,4,8,16,32,64,128,256)
 */
void spi_init(int mode, int frame_bits, unsigned int clk_div);

/**
 * spi_cs_low() / spi_cs_high() — manual chip select control
 * Call before/after each transaction.
 */
void spi_cs_low(void);
void spi_cs_high(void);

/**
 * spi_transfer() — send and receive 1 byte simultaneously
 * Full-duplex: TX and RX happen at the same time.
 * Returns received byte, or 0xFF on timeout.
 */
unsigned char spi_transfer(unsigned char tx);

/**
 * spi_transfer16() — send and receive 16-bit word
 */
unsigned short spi_transfer16(unsigned short tx);

/**
 * spi_write_buf() — send buffer, discard received bytes
 * @buf: data to send
 * @len: number of bytes
 * Returns SPI_OK or error code
 */
int spi_write_buf(const unsigned char *buf, unsigned int len);

/**
 * spi_read_buf() — receive buffer, send dummy 0xFF bytes
 * @buf: buffer to store received data
 * @len: number of bytes to receive
 * Returns SPI_OK or error code
 */
int spi_read_buf(unsigned char *buf, unsigned int len);

/**
 * spi_loopback_test() — test SPI with known pattern
 * Sends test bytes and verifies echo (requires MISO=MOSI).
 * Prints results via UART — used in diagnostics.
 * Returns number of errors (0 = pass).
 */
int spi_loopback_test(void);

/**
 * spi_error_str() — return human-readable error string
 */
const char *spi_error_str(int err);

#endif /* SPI_H */