#ifndef I2C_H
#define I2C_H

/**
 * i2c.h — I2C Master Driver
 * ============================================================
 * ARM Cortex-M bare-metal I2C master implementation
 * Direct register access — no HAL, no RTOS dependency
 *
 * Relevant for:
 *   THEON    — sensor bus (IMU, temperature, pressure)
 *   Renesas  — IoT sensor nodes (I2C sensor arrays)
 *   TechBiz  — HAL verification, board bring-up
 *
 * Supports:
 *   - 100 kHz Standard Mode
 *   - 400 kHz Fast Mode
 *   - 7-bit addressing
 *   - Single byte read/write
 *   - Multi-byte burst read/write
 *   - Timeout-based error detection (no infinite loops)
 *
 * Error codes:
 *   I2C_OK        (0)  — success
 *   I2C_ERR_NACK  (-1) — device not responding
 *   I2C_ERR_ARB   (-2) — bus arbitration lost
 *   I2C_ERR_TOUT  (-3) — timeout waiting for hardware
 *   I2C_ERR_BUS   (-4) — bus error (SDA/SCL stuck)
 * ============================================================
 */

/* ── Error codes ────────────────────────────────────────────── */
#define I2C_OK          0
#define I2C_ERR_NACK   -1
#define I2C_ERR_ARB    -2
#define I2C_ERR_TOUT   -3
#define I2C_ERR_BUS    -4

/* ── Speed modes ────────────────────────────────────────────── */
#define I2C_SPEED_STANDARD  100000UL   /* 100 kHz */
#define I2C_SPEED_FAST      400000UL   /* 400 kHz */

/* ── Public API ─────────────────────────────────────────────── */

/**
 * i2c_init() — initialize I2C peripheral
 * @speed: I2C_SPEED_STANDARD or I2C_SPEED_FAST
 */
void i2c_init(unsigned int speed);

/**
 * i2c_write_byte() — write single byte to device register
 * @dev_addr: 7-bit device address (e.g. 0x48 for temp sensor)
 * @reg:      register address on device
 * @data:     byte to write
 * Returns: I2C_OK or error code
 */
int i2c_write_byte(unsigned char dev_addr,
                   unsigned char reg,
                   unsigned char data);

/**
 * i2c_read_byte() — read single byte from device register
 * @dev_addr: 7-bit device address
 * @reg:      register address on device
 * @data:     pointer to store received byte
 * Returns: I2C_OK or error code
 */
int i2c_read_byte(unsigned char dev_addr,
                  unsigned char reg,
                  unsigned char *data);

/**
 * i2c_write_burst() — write multiple bytes starting at register
 * @dev_addr: 7-bit device address
 * @reg:      starting register address
 * @buf:      data buffer to write
 * @len:      number of bytes to write
 * Returns: I2C_OK or error code
 */
int i2c_write_burst(unsigned char  dev_addr,
                    unsigned char  reg,
                    const unsigned char *buf,
                    unsigned int   len);

/**
 * i2c_read_burst() — read multiple bytes starting at register
 * @dev_addr: 7-bit device address
 * @reg:      starting register address
 * @buf:      buffer to store received bytes
 * @len:      number of bytes to read
 * Returns: I2C_OK or error code
 */
int i2c_read_burst(unsigned char  dev_addr,
                   unsigned char  reg,
                   unsigned char *buf,
                   unsigned int   len);

/**
 * i2c_scan() — scan bus and print responding addresses via UART
 * Useful for field diagnostics (THEON use case)
 */
void i2c_scan(void);

/**
 * i2c_error_str() — return human-readable error string
 */
const char *i2c_error_str(int err);

#endif /* I2C_H */