# BMS I²C Reference Measurement Device

**Document:** `docs/bms_i2c_reference_device.md`
**Project:** ARM-MSV-Framework-RTOS
**Feature:** BMS-Oriented Embedded Software Foundation
**Status:** Specification
**Version:** 1.0
**Target Platform:** ARM Cortex-M3 / FreeRTOS / I²C

---

## 1. Purpose

This document defines the protocol contract for the **Reference BMS I²C Measurement Device**.

The purpose of this device is to provide a **deterministic, project-defined reference backend** for exercising the BMS I²C measurement abstraction (`bms_measurement_device_t`). It exists to:

- Enable host-based unit testing of the BMS measurement acquisition path without requiring physical hardware.
- Provide a stable protocol contract for the I²C adapter implementation.
- Demonstrate the integration between the I²C driver, the measurement abstraction, and the BMS manager.

**Important:** This device is **synthetic/reference** and does **not** represent any specific commercial battery-monitor IC, sensor, or real-world I²C device. It is a project-defined protocol used solely for software verification and integration testing.

A physical I²C device with identical behaviour may be implemented in simulation or hardware, but this specification does **not** guarantee the existence of such hardware.

---

## 2. I²C Protocol

### 2.1 Device Address

| Parameter     | Value            |
| ------------- | ---------------- |
| 7-bit address | `0x40`           |
| 8-bit write   | `0x80`           |
| 8-bit read    | `0x81`           |

The device responds only to address `0x40`. Other addresses are ignored (NACK).

### 2.2 Register Map

| Register Address | Parameter   | Size (bytes) | Access |
| ---------------- | ----------- | ------------ | ------ |
| `0x00`           | Voltage     | 2            | Read   |
| `0x02`           | Current     | 2            | Read   |
| `0x04`           | Temperature | 2            | Read   |

Total register space: 6 bytes (0x00–0x05).

The device does not support write operations. Any write attempt is ignored (NACK).

### 2.3 Byte Order

All 16-bit values are stored in **big-endian** order:

```text
MSB first, LSB second.
```

Example:

```text
0x3039 → 0x30 (MSB), 0x39 (LSB)
```

### 2.4 Reading Data

To read all three parameters in a single transaction:

```text
Start → Write 8-bit address (0x80) → Write register (0x00) → Repeated Start → Read 8-bit address (0x81) → Read 6 bytes → Stop
```

The device responds with 6 bytes sequentially:

```text
Byte 0: Voltage MSB
Byte 1: Voltage LSB
Byte 2: Current MSB
Byte 3: Current LSB
Byte 4: Temperature MSB
Byte 5: Temperature LSB
```

Reading individual registers is not supported by this protocol. The device expects burst reads starting at register 0x00.

---

## 3. Parameter Encoding

### 3.1 Voltage

| Parameter | Unit         | Scaling       | Format    | Range                 |
| --------- | ------------ | ------------- | --------- | --------------------- |
| Voltage   | Volts (V)    | raw / 1000.0  | unsigned  | 0.000 V to 65.535 V   |

**Example:**

```text
raw = 0x3039 = 12345
voltage = 12345 / 1000.0 = 12.345 V
```

### 3.2 Current

| Parameter | Unit         | Scaling       | Format    | Range                   |
| --------- | ------------ | ------------- | --------- | ----------------------- |
| Current   | Amperes (A)  | raw / 1000.0  | unsigned  | 0.000 A to 65.535 A     |

**Example:**

```text
raw = 0x01F4 = 500
current = 500 / 1000.0 = 0.500 A
```

### 3.3 Temperature

| Parameter   | Unit         | Scaling       | Format    | Range                   |
| ----------- | ------------ | ------------- | --------- | ----------------------- |
| Temperature | Degrees C    | raw / 10.0    | unsigned  | 0.0 °C to 6553.5 °C     |

**Example:**

```text
raw = 0x0190 = 400
temperature = 400 / 10.0 = 40.0 °C
```

---

## 4. Valid Ranges

The device is defined with the following valid operating ranges:

| Parameter   | Min          | Max          |
| ----------- | ------------ | ------------ |
| Voltage     | 0.0 V        | 65.535 V     |
| Current     | 0.0 A        | 65.535 A     |
| Temperature | 0.0 °C       | 6553.5 °C    |

All values outside these ranges are not representable in the protocol. The adapter shall interpret raw values as valid measurements without further range checking; range validation is the responsibility of the BMS protection layer.

---

## 5. Communication Failure Semantics

The device must respond deterministically to communication errors. The following failure classes are defined, mapping to the actual `i2c.h` error constants:

| I²C Error          | Adapter Result               |
| ------------------ | ---------------------------- |
| `I2C_ERR_NACK`     | `BMS_MEAS_DEVICE_ERROR`      |
| `I2C_ERR_TOUT`     | `BMS_MEAS_DEVICE_ERROR`      |
| `I2C_ERR_BUS`      | `BMS_MEAS_DEVICE_ERROR`      |
| `I2C_ERR_ARB`      | `BMS_MEAS_DEVICE_ERROR`      |

**Important:** The public BMS measurement device abstraction (`bms_measurement_device.h`) does not expose per-error classifications. All I²C communication failures are reported as a generic `BMS_MEAS_DEVICE_ERROR`. Detailed error codes remain internal to the adapter and test layer.

### 5.1 Success Path

- When all bytes are received successfully and the status is `I2C_OK`, the adapter shall decode the raw bytes and produce `bms_measurements_t` with:
  - `voltage.status = BMS_MEAS_VALID`
  - `current.status = BMS_MEAS_VALID`
  - `temperature.status = BMS_MEAS_VALID`
- If any error occurs, the adapter shall set:
  - `measurements->voltage.status = BMS_MEAS_INVALID`
  - `measurements->current.status = BMS_MEAS_INVALID`
  - `measurements->temperature.status = BMS_MEAS_INVALID`
  - All values set to `0.0f`.
- Return `BMS_MEAS_DEVICE_ERROR`.

---

## 6. Adapter Interface

The concrete adapter shall implement the callback prototype defined in `bms_measurement_device.h`:

```c
typedef bms_measurement_device_status_t
(*bms_measurement_device_read_fn)(
    bms_measurements_t *measurements,
    void *context
);
```

The context structure shall contain at least:

```c
typedef struct {
    unsigned char dev_addr;   /* 7-bit I²C device address */
    unsigned int timeout_ms;  /* I²C timeout in milliseconds (optional) */
} bms_i2c_measurement_context_t;
```

The adapter shall use `i2c_read_burst()` to read the 6 bytes from the device:

```c
i2c_read_burst(
    context->dev_addr,
    0x00,           /* start register */
    raw_data,       /* 6-byte buffer */
    6               /* length */
);
```

---

## 7. Test Strategy

### 7.1 Unit Tests (host-based, mocked I²C)

The adapter shall be tested with a mocked `i2c_read_burst()`:

- **Success path:** Mock returns 6 valid raw bytes → Verify decoded measurements.
- **NACK:** Mock returns `I2C_ERR_NACK` → Verify `BMS_MEAS_DEVICE_ERROR` and invalid measurements.
- **Timeout:** Mock returns `I2C_ERR_TOUT` → Verify `BMS_MEAS_DEVICE_ERROR`.
- **Bus error:** Mock returns `I2C_ERR_BUS` → Verify `BMS_MEAS_DEVICE_ERROR`.
- **Arbitration error:** Mock returns `I2C_ERR_ARB` → Verify `BMS_MEAS_DEVICE_ERROR`.
- **NULL context:** Verify graceful rejection.

### 7.2 Integration Tests (QEMU, real I²C emulation)

Using QEMU or a simulated I²C backend:

- Configure the device emulation to match the reference protocol.
- Perform a full BMS cycle:
  - Read measurements from the emulated device.
  - Verify BMS protection and state updates.
  - Inject communication errors and verify fault handling.

**Important:** These tests exercise the I²C driver and the adapter together, but do **not** claim physical hardware validation.

---

## 8. Scope and Limitations

### 8.1 In Scope

- Software protocol definition.
- Adapter implementation using existing `i2c_read_burst()`.
- Unit and integration tests using mocked/simulated I²C.
- Connection to the BMS manager via `bms_measurement_device_t`.

### 8.2 Out of Scope

- Physical battery hardware.
- Real battery-monitor IC.
- Hardware-in-the-loop validation.
- Electrical safety validation.
- Production sensor calibration.

### 8.3 Scope Note

This reference device does **not** claim to represent a real-world battery-monitor IC. It is a project-defined synthetic protocol designed for software verification and integration testing. The implementation and tests do **not** constitute validation of any physical battery measurement hardware.

---

## 9. Revision History

| Version | Date       | Author         | Changes                     |
| ------- | ---------- | -------------- | --------------------------- |
| 1.0     | 2026-08-30 | Project Team   | Initial specification.      |

---

## 10. References

- `src/bms/bms_measurement_device.h` – Measurement device abstraction.
- `src/drivers/i2c.h` – I²C driver API.
- `tests/unit/test_bms_i2c_measurement_device.c` – Adapter unit tests.
- `docs/bms_architecture.md` – BMS software architecture.
```

---

## Σύνοψη διορθώσεων

1. **Error names** – χρησιμοποιούνται τα σωστά project symbols:
   - `I2C_ERR_TOUT` (όχι `I2C_ERR_TIMEOUT`)
   - `I2C_ERR_ARB` (όχι `I2C_ERR_ARBITRATION`)
2. **Callback type** – διορθώθηκε σε `bms_measurement_device_read_fn` (χωρίς `_t`).
3. **Public error classification** – ξεκαθαρίζεται ότι όλα τα I²C errors μεταφράζονται σε `BMS_MEAS_DEVICE_ERROR` και ότι τα λεπτομερή error codes είναι εσωτερικά.

---

## Επόμενο βήμα

Η specification είναι πλέον συνεπής με το πραγματικό repository.
Είμαστε έτοιμοι να προχωρήσουμε στην υλοποίηση:

```text
src/bms/bms_i2c_measurement_device.h
src/bms/bms_i2c_measurement_device.c
tests/unit/test_bms_i2c_measurement_device.c
