# ARM MSV OS: Real-Time Embedded Firmware Platform

A production-oriented embedded firmware framework for **ARM Cortex-M3 (LM3S6965)**, combining FreeRTOS-based real-time execution, register-level peripheral drivers, GNSS/NMEA processing, communication interfaces, watchdog supervision, fault handling, runtime diagnostics, and automated QEMU integration testing.

The project focuses on **low-level embedded C development, deterministic real-time execution, peripheral integration, firmware diagnostics, fault recovery, and reproducible verification**.

The v2.6 development baseline emphasizes reproducible builds, automated testing, CLI observability, interrupt-driven I/O, and incremental regression closure.

---

## 🚀 Key Features

### 1. Real-Time Embedded Firmware

* C-based firmware targeting ARM Cortex-M3.
* FreeRTOS-based preemptive multitasking.
* CLI, application, AI-inference and system-monitor tasks.
* Runtime task-state and priority observability.
* Stack high-water-mark monitoring.
* Heap and system-uptime monitoring.
* Deterministic firmware execution under QEMU.

---

### 2. Communication & Peripheral Drivers

Register-level embedded drivers developed without a vendor HAL, including:

* UART
* GPIO
* I2C
* SPI
* Timer
* Watchdog
* ESP8266 interface
* GNSS/NMEA processing infrastructure

The firmware uses direct peripheral-register access with explicit hardware configuration and interrupt-aware driver design.

The UART subsystem includes:

* Register-level UART0 configuration.
* 115200-baud configuration for the LM3S6965 target.
* Blocking TX path.
* Thread-safe string transmission using a FreeRTOS mutex.
* Interrupt-driven RX path.
* UART RX hardware FIFO draining from the interrupt handler.
* Software RX ring buffer for decoupling interrupt-driven reception from task-level consumption.

The interrupt-driven UART RX implementation is currently undergoing QEMU regression validation before being promoted to a stable baseline.

---

## 3. GNSS / NMEA Processing

The firmware contains a modular GNSS/NMEA processing subsystem supporting:

* NMEA sentence processing.
* GGA parsing.
* RMC parsing.
* Position and navigation data extraction.
* Parser-level automated testing.
* Separation of protocol parsing from the RTOS application layer.

The GNSS subsystem is intentionally structured so that protocol parsing can be developed and tested independently from the physical GNSS receiver.

### External GNSS Receiver Integration — Next Development Phase

The next planned GNSS development phase is integration with an **external GNSS receiver**.

The intended architecture is:

```text
External GNSS Receiver
        │
        │ UART
        ▼
ARM Cortex-M3 UART Driver
        │
        ▼
Interrupt-Driven RX
        │
        ▼
Software RX Ring Buffer
        │
        ▼
GNSS/NMEA Parser
        │
        ├── GGA
        ├── RMC
        └── Navigation Data
```

The existing UART interrupt-driven RX architecture provides the foundation for this integration.

The GNSS hardware-integration phase will focus on:

* Physical UART integration with an external GNSS receiver.
* Reliable asynchronous NMEA reception.
* Continuous RX stream handling.
* Integration of the UART RX ring buffer with the GNSS parser.
* NMEA message validation.
* Receiver-to-parser integration testing.
* Robust handling of continuous GNSS data streams.
* Separation between transport, protocol parsing and application-level navigation data.

**External GNSS hardware integration is not yet claimed as verified functionality in the current v2.6 baseline.**

It will be promoted to the verified feature set only after the corresponding integration and regression tests are completed.

---

## 4. Reliability, Diagnostics & Recovery

The firmware implements software health monitoring and fault-recovery mechanisms for ARM Cortex-M3.

Features include:

* Watchdog and heartbeat-based task supervision.
* Software health monitoring.
* ARM Cortex-M fault handling.
* HardFault diagnostics.
* Software recovery through the ARM Cortex-M system reset mechanism.
* Persistent post-mortem information using a `.noinit` RAM section.
* Runtime diagnostic CLI.
* Deliberate fault-injection commands for recovery testing.

The recovery architecture is:

```text
Health Monitoring
       ↓
Fault / Timeout Detection
       ↓
Recovery Handler
       ↓
AIRCR System Reset
       ↓
Persistent .noinit Diagnostics
       ↓
Normal Firmware Boot
```

---

## 5. Verification & Emulation

The project uses a layered verification approach combining:

* ARM Cortex-M3 firmware execution under QEMU.
* Telnet/serial CLI integration testing.
* Pytest-based black-box firmware verification.
* Automated clean-build verification.
* Runtime observability testing.
* Fault and recovery test infrastructure.
* Verilator-based peripheral simulation.
* GitHub Actions build and integration-test workflow.

The verification workflow distinguishes between:

1. Firmware build correctness.
2. Individual test behaviour.
3. Full-suite regression status.
4. End-to-end QEMU runtime behaviour.
5. Known verification gaps.

This prevents unstable runtime behaviour from being incorrectly reported as verified functionality.

---

## 6. Edge AI / TinyML

The platform also contains a lightweight C-based TinyML inference component for predictive-maintenance temperature estimation.

The inference runs locally on the Cortex-M3 target without cloud connectivity.

The TinyML subsystem is integrated into the RTOS task architecture and exposed through the runtime diagnostic CLI.

---

## 📂 Project Architecture

```text
ARM-MSV-Framework-RTOS/

├── .github/
│   └── workflows/
│       └── ci.yml              # Build and QEMU integration CI
│
├── docs/                       # Verification and engineering documentation
├── scripts/                    # Development and ML utilities
├── sim/                        # Verilator simulation environment
│
├── src/
│   ├── arch/arm/               # ARM startup and system support
│   ├── drivers/                # UART, GPIO, I2C, SPI, WDT, etc.
│   ├── fpga/                   # FPGA/Verilog simulation models
│   ├── gnss/                   # GNSS/NMEA parser
│   ├── kernel/                 # CLI, diagnostics, TinyML integration
│   ├── rtos/                   # FreeRTOS configuration and sources
│   └── utils/                  # Utility functions
│
├── tests/
│   ├── conftest.py             # Shared QEMU/test infrastructure
│   ├── test_automation.py      # Core automation tests
│   ├── test_cli.py             # CLI integration tests
│   ├── test_cli_observability.py
│   └── test_recovery.py        # Recovery tests
│
├── linker.ld                   # Firmware linker script
├── Makefile                    # Firmware build system
├── setup_rtos.sh               # RTOS setup helper
└── README.md
```

---

## 🛠️ Technology Stack

| Area                        | Technology                             |
| --------------------------- | -------------------------------------- |
| MCU target                  | ARM Cortex-M3                          |
| Emulated board              | LM3S6965                               |
| RTOS                        | FreeRTOS                               |
| Firmware language           | C                                      |
| Low-level language          | ARM Assembly                           |
| Peripheral interfaces       | UART, I2C, SPI, GPIO, Timer            |
| GNSS protocol               | NMEA                                   |
| GNSS hardware integration   | External receiver — planned next phase |
| Wireless interface          | ESP8266                                |
| Hardware description        | Verilog                                |
| Automation / ML             | Python                                 |
| Compiler                    | `arm-none-eabi-gcc`                    |
| Build system                | GNU Make                               |
| CPU emulation               | QEMU                                   |
| RTL / peripheral simulation | Verilator                              |
| Test framework              | Pytest                                 |
| Version control             | Git                                    |
| CI                          | GitHub Actions                         |

---

## 💻 Runtime Diagnostic CLI

The firmware exposes an interactive diagnostic CLI through the QEMU serial/Telnet interface.

| Command    | Description                                                |
| ---------- | ---------------------------------------------------------- |
| `help`     | Display available commands                                 |
| `ps`       | Display RTOS task states, priorities and stack information |
| `stack`    | Display task stack high-water marks                        |
| `mem`      | Display current and minimum-ever free heap                 |
| `uptime`   | Display system execution time                              |
| `predict`  | Run TinyML temperature prediction                          |
| `boost_ai` | Raise AI task priority                                     |
| `low_ai`   | Lower AI task priority                                     |
| `freeze`   | Exercise task-hang recovery                                |
| `crash`    | Exercise processor-fault recovery                          |

The CLI acts as a lightweight **firmware diagnostic and runtime-observability interface**.

---

## ⚙️ Build

Install the ARM GNU toolchain and QEMU for ARM.

Build the firmware:

```bash
make clean
make
```

The resulting firmware image is:

```text
firmware.elf
```

Inspect the firmware size with:

```bash
arm-none-eabi-size firmware.elf
```

### Latest locally verified clean build

```text
[LINKING] firmware.elf

text    data    bss     dec     hex
32060   84      10908   43052   a82c
```

The firmware currently builds successfully with the interrupt-driven UART RX implementation present.

---

## 🖥️ Run under QEMU

Launch the Cortex-M3 target:

```bash
qemu-system-arm \
    -M lm3s6965evb \
    -nographic \
    -serial telnet:127.0.0.1:4444,server,nowait \
    -kernel firmware.elf
```

The firmware CLI is exposed through TCP port `4444`.

---

## 🧪 Automated Verification

The project uses **Pytest together with QEMU** for black-box firmware integration testing.

Run the complete regression suite:

```bash
pytest -q
```

### Current verification status

Following the introduction of the interrupt-driven UART RX path, the latest clean-build regression run reports:

```text
8 passed, 9 failed
```

The firmware itself builds successfully.

The current failures are concentrated around **UART command reception and command-stream synchronization**, affecting CLI and command-driven recovery tests.

Observed examples include:

```text
help      → he

mem       → incomplete RX data

freeze    → fr
```

This indicates an active UART RX regression rather than an isolated CLI parser failure.

The current UART implementation uses:

```text
UART hardware FIFO
        ↓
UART0_IRQHandler()
        ↓
software RX ring buffer
        ↓
uart_getc()
        ↓
CLI task
```

The interrupt handler:

* Reads UART interrupt status.
* Handles RX and receive-timeout interrupts.
* Drains the hardware RX FIFO.
* Pushes received bytes into the software RX ring buffer.
* Clears the corresponding interrupt sources.

The software RX buffer uses a power-of-two ring-buffer design with separate producer and consumer indices.

The implementation deliberately avoids calling FreeRTOS APIs from the UART ISR.

The issue is currently being investigated and the interrupt-driven RX implementation is **not yet considered regression-closed**.

The verification workflow deliberately distinguishes between:

1. Firmware build correctness.
2. Individual test behaviour.
3. Full-suite regression status.
4. End-to-end QEMU runtime behaviour.
5. Known verification gaps.

---

## 🔬 v2.6 Runtime Verification

The project maintains a dedicated runtime verification record:

```text
docs/v2.6_runtime_verification.md
```

The document contains historical and current runtime evidence from:

1. Automated Pytest/QEMU integration testing.
2. Manual QEMU CLI verification.

Manual verification has been used to exercise:

* RTOS task observability.
* Stack monitoring.
* Heap monitoring.
* System timing.
* TinyML inference.
* Watchdog/task-hang recovery.
* HardFault recovery.

Current automated regression status is reported separately above so that historical runtime evidence is not confused with the current UART RX regression state.

---

## 🛡️ Reliability, Fault Handling & Recovery

The firmware implements software health monitoring and fault-recovery mechanisms for ARM Cortex-M3.

The recovery architecture is:

```text
Health Monitoring
       ↓
Fault / Timeout Detection
       ↓
Recovery Handler
       ↓
AIRCR System Reset
       ↓
Persistent .noinit Diagnostics
       ↓
Normal Firmware Boot
```

### Watchdog / Task-Hang Recovery

The `freeze` command is designed to suspend the CLI task and exercise the software health-monitor recovery path.

### HardFault Recovery

The `crash` command deliberately exercises the processor fault-handling path.

Dedicated automated recovery tests are implemented in:

```text
tests/test_recovery.py
```

The current UART RX regression affects command-driven recovery tests, therefore these paths remain active verification items until the command-stream issue is resolved.

### Hardware Watchdog Qualification

The LM3S6965 hardware watchdog driver is implemented as a register-level driver according to the target peripheral model.

A dedicated QEMU experiment verified watchdog register access and counter expiry.

However, QEMU's `lm3s6965evb` model does not provide a reliable firmware-level Cortex-M reboot when the watchdog expires.

Therefore, the project does **not** claim hardware watchdog reset as a fully verified QEMU recovery mechanism.

The software recovery mechanism used by the v2.6 framework is:

```text
Software Health Monitor
        ↓
Fault / Recovery Handler
        ↓
AIRCR System Reset
        ↓
Post-Mortem Diagnostic Information
        ↓
Normal Firmware Boot
```

---

## 📊 Runtime Observability

The firmware provides runtime observability through the diagnostic CLI.

### RTOS Tasks

```text
rtos_msv> ps
```

Reports task state and priority information.

### Stack Monitoring

```text
rtos_msv> stack
```

Reports stack high-water-mark information.

### Heap Monitoring

```text
rtos_msv> mem
```

Reports current and minimum-ever free heap.

### System Timing

```text
rtos_msv> uptime
```

Reports system execution time using the FreeRTOS timing infrastructure.

These interfaces form the runtime diagnostics layer used by the QEMU integration tests.

---

## 🔧 Engineering & Development Practices

The project follows an incremental embedded-firmware development workflow based on:

* Modular C firmware architecture.
* Register-level peripheral development.
* RTOS task and resource management.
* Interrupt-driven I/O.
* Git-based version control.
* Reproducible clean builds.
* Automated regression testing.
* QEMU-based target-level integration testing.
* Fault injection and recovery testing.
* Runtime diagnostics and observability.
* Root-cause analysis of integration failures.
* Regression closure before baseline promotion.
* Hardware-independent protocol parsing.
* Incremental integration of external peripherals.

The development process intentionally separates:

```text
Implemented
    ↓
Built
    ↓
Individually Tested
    ↓
Integrated
    ↓
Regression Verified
    ↓
Baseline
```

This prevents partially integrated functionality from being presented as production-verified.

---

## 🔧 CI Verification

GitHub Actions validates the firmware build and integration-test workflow on repository changes.

The CI pipeline performs:

1. Repository checkout.
2. ARM GNU toolchain setup.
3. QEMU setup.
4. Python environment setup.
5. Pytest installation.
6. Clean firmware build.
7. ELF validation.
8. QEMU integration testing.

The workflow is:

```text
Source
  ↓
Clean Build
  ↓
firmware.elf
  ↓
QEMU Cortex-M3
  ↓
Pytest Integration Tests
  ↓
Regression Result
```

Workflow definition:

```text
.github/workflows/ci.yml
```

---

## 🔬 Peripheral Simulation

The repository also contains a Verilator-based SPI simulation environment.

Build and run the simulation:

```bash
cd sim
make -f Makefile.sim
./obj_dir/sim_executable
```

Generated Verilator build artifacts are intentionally excluded from Git.

---

# 📌 Current Development Status

## Verified / Established

* ARM Cortex-M3 / LM3S6965 firmware build.
* FreeRTOS integration.
* Register-level peripheral architecture.
* UART TX infrastructure.
* UART driver configuration and initialization.
* GPIO, I2C and SPI driver infrastructure.
* GNSS/NMEA parser infrastructure.
* GGA and RMC parsing.
* Runtime diagnostic CLI.
* QEMU firmware boot.
* Automated test infrastructure.
* Git-based reproducible development workflow.
* Watchdog and fault-handling infrastructure.
* TinyML inference integration.
* ARM Cortex-M fault diagnostics.
* Software recovery architecture.
* `.noinit` post-mortem diagnostic mechanism.
* Interrupt-vector integration for UART0.
* Interrupt-driven UART RX implementation.
* Software RX ring-buffer architecture.

## Active Verification Item

The current development branch contains an **interrupt-driven UART RX implementation** using:

```text
UART0
  ↓
UART0_IRQHandler()
  ↓
Hardware RX FIFO
  ↓
Software RX Ring Buffer
  ↓
uart_getc()
  ↓
CLI
```

The latest clean firmware build succeeds.

The current full QEMU regression result is:

```text
8 passed, 9 failed
```

The failures are concentrated around UART command reception and command-stream synchronization.

Therefore:

> **The interrupt-driven UART RX path is implemented but not yet regression-closed.**

The immediate engineering objective is:

```text
UART RX investigation
        ↓
Root-cause identification
        ↓
RX / CLI regression fix
        ↓
Full-suite regression
        ↓
Stable v2.6 UART baseline
```

Only after the regression suite is stable should the UART RX implementation be promoted as a fully verified v2.6 baseline feature.

---

# 🛰️ Next Development Phase — External GNSS Receiver Integration

After the UART RX regression is closed, the next major development phase is integration with an **external GNSS receiver**.

The planned integration builds directly on the existing architecture:

```text
External GNSS Receiver
      │
      │ UART / NMEA
      ▼
LM3S6965 UART0
      │
      ▼
Interrupt-Driven RX
      │
      ▼
Software RX Ring Buffer
      │
      ▼
GNSS/NMEA Parser
      │
      ▼
GGA / RMC
      │
      ▼
Navigation Data
      │
      ▼
Application / Diagnostics
```

The GNSS work will be developed incrementally.

### Phase 1 — UART Transport Validation

* Stabilize interrupt-driven UART RX.
* Close QEMU CLI regression.
* Verify continuous byte reception.
* Verify ring-buffer behaviour.
* Verify command-stream synchronization.
* Establish a stable UART transport baseline.

### Phase 2 — External GNSS Receiver Integration

* Connect an external GNSS receiver through UART.
* Receive real NMEA traffic.
* Validate sentence boundaries.
* Validate continuous asynchronous reception.
* Feed received data into the existing GNSS parser.

### Phase 3 — GNSS Integration Verification

* Validate GGA messages.
* Validate RMC messages.
* Validate position extraction.
* Validate navigation-data propagation.
* Add GNSS integration tests.
* Exercise receiver/parser behaviour under continuous data flow.

### Phase 4 — Runtime Integration

The final architecture will allow GNSS information to participate in the existing RTOS runtime environment:

```text
External GNSS Receiver
     ↓
UART ISR
     ↓
RX Ring Buffer
     ↓
GNSS Parser
     ↓
Navigation Data
     ↓
RTOS Application
     ↓
Diagnostics / Monitoring
```

The external GNSS receiver integration is **planned functionality and is not currently claimed as verified hardware functionality**.

The existing GNSS/NMEA parser remains a modular software subsystem that can be validated independently of the physical receiver.

---

## 🎯 Development Roadmap

```text
v2.6
 │
 ├── Reproducible firmware build              ✓
 ├── QEMU execution                           ✓
 ├── FreeRTOS runtime                         ✓
 ├── Runtime diagnostic CLI                   ✓
 ├── Fault / recovery infrastructure          ✓
 ├── TinyML integration                       ✓
 ├── GNSS/NMEA parser                         ✓
 ├── UART interrupt-driven RX                 ✓ implemented
 │
 └── UART RX regression closure               → ACTIVE
                                                  │
                                                  ▼
                                        Stable UART baseline
                                                  │
                                                  ▼
                                  External GNSS integration
                                                  │
                                                  ├── UART transport
                                                  ├── NMEA stream
                                                  ├── GGA/RMC validation
                                                  ├── Integration tests
                                                  └── Runtime GNSS
```

The project intentionally follows this progression rather than claiming the final GNSS hardware integration before the underlying transport and integration layers have been verified.

---

## 📌 Engineering Status Summary

The current project represents a **working ARM Cortex-M3 / FreeRTOS embedded firmware platform with register-level peripheral drivers, runtime diagnostics, fault recovery, TinyML integration, GNSS/NMEA processing, QEMU-based execution and automated verification infrastructure**.

The immediate priority is **regression closure of the interrupt-driven UART RX implementation**.

Once the UART transport is stable, the next development stage is **external GNSS receiver integration**, using the existing interrupt-driven UART and GNSS/NMEA architecture as the foundation.

The project therefore maintains a clear separation between:

```text
Verified infrastructure
        ↓
Implemented but under verification
        ↓
Planned hardware integration
```

This status is deliberately maintained to ensure that the repository reflects actual engineering evidence rather than aspirational functionality.
