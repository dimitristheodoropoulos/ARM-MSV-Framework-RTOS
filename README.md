# ARM MSV OS: High-Reliability RTOS Framework + Edge AI

A production-oriented embedded systems framework for **ARM Cortex-M3 (LM3S6965)**, combining FreeRTOS-based real-time execution, bare-metal drivers, TinyML inference, watchdog supervision, fault handling, and automated QEMU integration testing.

The **v2.6 production baseline** focuses on reproducible builds, automated verification, CLI observability, and runtime recovery validation.

---

## 🚀 Key Features

### 1. Real-Time RTOS

- FreeRTOS-based preemptive multitasking.
- CLI, AI inference, and system-monitor tasks.
- Runtime AI task-priority control.
- Task-state and priority observability through the CLI.
- System uptime monitoring.

### 2. High-Reliability and Self-Healing

- Watchdog supervision for task hangs and heartbeat failures.
- Fault recovery through ARM Cortex-M system reset mechanisms.
- Persistent post-mortem information using a `.noinit` RAM section.
- Custom HardFault handling for invalid processor states and memory accesses.
- CLI commands for deliberately exercising fault-recovery paths.
- Verified software recovery paths for watchdog/task-hang and HardFault scenarios.

### 3. Edge AI / TinyML

- Lightweight C-based TinyML inference.
- Linear-regression model for predictive-maintenance temperature estimation.
- Runtime prediction through the `predict` CLI command.
- Designed for execution without cloud connectivity.

### 4. Bare-Metal Drivers

Register-level embedded drivers for:

- UART
- GPIO
- I2C
- SPI
- Timer
- Watchdog
- ESP8266 interface
- GNSS/NMEA parsing

The firmware is designed around direct hardware access without a vendor HAL.

### 5. Verification and Emulation

- ARM Cortex-M3 firmware emulation with QEMU.
- Telnet/serial CLI integration testing.
- Pytest-based black-box verification.
- Manual QEMU runtime verification for recovery and observability paths.
- Verilator-based SPI slave simulation.
- Automated GitHub Actions build and integration-test pipeline.

---

## 📂 Project Architecture

```text
ARM-MSV-Framework-RTOS/
├── .github/
│   └── workflows/
│       └── ci.yml              # Build and QEMU integration CI
├── docs/                       # Verification, hardware and protocol documentation
├── scripts/                    # Development and ML utilities
├── sim/                        # Verilator simulation environment
├── src/
│   ├── arch/arm/               # ARM startup and system support
│   ├── drivers/                # UART, GPIO, I2C, SPI, WDT, etc.
│   ├── fpga/                   # FPGA/Verilog simulation models
│   ├── gnss/                   # GNSS/NMEA parser
│   ├── kernel/                 # CLI, diagnostics, TinyML integration
│   ├── rtos/                   # FreeRTOS configuration and sources
│   └── utils/                  # Utility functions
├── tests/
│   ├── conftest.py             # Shared QEMU/test infrastructure
│   ├── test_automation.py      # Core automation tests
│   ├── test_cli.py             # CLI integration tests
│   ├── test_cli_observability.py
│   └── test_recovery.py        # Watchdog and HardFault recovery tests
├── linker.ld                   # Firmware linker script
├── Makefile                    # Firmware build system
├── setup_rtos.sh               # RTOS setup helper
└── README.md
```

---

## 🛠️ Technology Stack

| Area                 | Technology          |
| -------------------- | ------------------- |
| MCU target           | ARM Cortex-M3       |
| Emulated board       | LM3S6965            |
| RTOS                 | FreeRTOS            |
| Firmware language    | C                   |
| Low-level language   | ARM Assembly        |
| Hardware description | Verilog             |
| ML / automation      | Python              |
| Compiler             | `arm-none-eabi-gcc` |
| Build system         | GNU Make            |
| CPU emulation        | QEMU                |
| RTL simulation       | Verilator           |
| Test framework       | Pytest              |
| CI                   | GitHub Actions      |

---

## 💻 CLI

The firmware exposes an interactive CLI through the QEMU serial/Telnet interface.

| Command    | Description                                                |
| ---------- | ---------------------------------------------------------- |
| `help`     | Display available commands                                 |
| `ps`       | Display RTOS task states, priorities and stack information |
| `stack`    | Display task stack high-water marks                        |
| `mem`      | Display current and minimum-ever free heap                 |
| `uptime`   | Display system execution time                              |
| `predict`  | Run/display TinyML temperature prediction                  |
| `boost_ai` | Raise AI task priority to level 4                          |
| `low_ai`   | Lower AI task priority to level 1                          |
| `freeze`   | Intentionally suspend the CLI task for watchdog testing    |
| `crash`    | Intentionally trigger a processor fault for recovery test  |

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

Latest locally verified clean-build footprint:

```text
text    data    bss     dec     hex
30816   84      10644   41544   a248
```

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

The CLI is exposed through TCP port `4444`.

---

## 🧪 Automated Verification

The v2.6 verification baseline uses pytest together with QEMU.

Run the complete regression suite:

```bash
pytest -q
```

Latest locally verified result:

```text
17 passed in 34.88s
```

The automated regression suite covers:

* CLI functionality
* Memory reporting
* RTOS task reporting
* System uptime
* TinyML inference
* AI task-priority control
* Invalid-command handling
* GNSS/NMEA parsing
* GNSS GGA parsing
* GNSS RMC parsing
* Watchdog/software recovery
* HardFault recovery

The recovery-specific tests are implemented in:

```text
tests/test_recovery.py
```

The shared QEMU recovery infrastructure is implemented in:

```text
tests/conftest.py
```

---

## 🔬 v2.6 Runtime Verification

The v2.6 baseline also includes a dedicated runtime verification record:

```text
docs/v2.6_runtime_verification.md
```

This document records runtime evidence from both:

1. Automated pytest integration testing.
2. Manual QEMU CLI verification.

The manual verification exercises:

| Runtime Area                | Test      | Result |
| --------------------------- | --------- | ------ |
| RTOS task management        | `ps`      | PASS   |
| Stack monitoring            | `stack`   | PASS   |
| Heap monitoring             | `mem`     | PASS   |
| System timing               | `uptime`  | PASS   |
| TinyML inference            | `predict` | PASS   |
| Watchdog/task-hang recovery | `freeze`  | PASS   |
| HardFault recovery          | `crash`   | PASS   |

The manual tests are intended to provide explicit black-box runtime evidence in addition to the automated regression suite.

---

## 🛡️ Recovery and Watchdog Verification

The project implements a software health-monitor recovery path using the ARM Cortex-M system reset mechanism (AIRCR), with persistent post-mortem information retained through the `.noinit` RAM section.

### Watchdog / Task-Hang Recovery

The `freeze` CLI command deliberately suspends the CLI task and exercises the software health-monitor recovery path.

The expected recovery sequence is:

```text
[TEST] Freezing CLI...

[KERNEL] EMERGENCY RESET...

[BOOT] Diagnostic Log (v2.3):
 -> STATUS: WATCHDOG RECOVERY
 -> CAUSE:  Task Hang (Heartbeat Timeout)
 -> TOTAL RESETS: 1
```

This verifies the software watchdog/heartbeat recovery path under QEMU.

### HardFault Recovery

The `crash` CLI command deliberately triggers a processor fault.

The verified QEMU runtime result is:

```text
[TEST] Forcing Usage Fault...

[KERNEL] EMERGENCY RESET...

[BOOT] Diagnostic Log (v2.3):
 -> STATUS: HARD FAULT RECOVERY
 -> CAUSE:  CPU HardFault (Invalid Access)
 -> TOTAL RESETS: 1
```

This confirms that the fault path reaches the emergency recovery handler, records persistent diagnostic information, performs the system reset, and boots back into the CLI.

### Hardware Watchdog Qualification

The LM3S6965 hardware watchdog driver is implemented as a register-level driver according to the target peripheral model.

A dedicated QEMU experiment verified watchdog register access and counter expiry.

However, QEMU's `lm3s6965evb` model does not provide a reliable firmware-level Cortex-M reboot when the watchdog expires. Therefore, the project does **not** claim hardware watchdog reset as a fully verified QEMU recovery mechanism.

The verified recovery mechanism for the v2.6 baseline is the:

```text
software health monitor
        ↓
fault / recovery handler
        ↓
AIRCR system reset
        ↓
post-mortem diagnostic log
        ↓
normal firmware boot
```

---

## 🔄 Recovery Verification Summary

The two primary software recovery paths are independently exercised:

| Test     | Recovery Path                | Result |
| -------- | ---------------------------- | ------ |
| `freeze` | Watchdog / heartbeat timeout | PASS   |
| `crash`  | Cortex-M HardFault recovery  | PASS   |

The recovery tests verify not only that an emergency reset occurs, but also that the post-reset diagnostic log identifies the recovery cause and reset count.

---

## 🤖 TinyML Runtime Verification

The firmware includes a lightweight C-based TinyML inference path for predictive-maintenance temperature estimation.

The `predict` CLI command executes the inference path directly on the embedded target.

Example usage:

```text
rtos_msv> predict
```

The TinyML path is included in the automated QEMU regression suite and the runtime verification baseline.

---

## 📊 Runtime Observability

The firmware provides runtime observability through the CLI.

### RTOS Tasks

```text
rtos_msv> ps
```

The command reports task state and priority information for the active RTOS tasks.

### Stack Monitoring

```text
rtos_msv> stack
```

The command reports stack high-water-mark information for the monitored tasks.

### Heap Monitoring

```text
rtos_msv> mem
```

The command reports current and minimum-ever free heap information.

### System Timing

```text
rtos_msv> uptime
```

The command reports system execution time based on the FreeRTOS timing infrastructure.

These interfaces are verified by the automated regression suite and the v2.6 runtime verification workflow.

---

## 🔧 CI Verification

GitHub Actions performs the production verification workflow on pushes and pull requests.

The CI pipeline performs:

1. Repository checkout
2. ARM GNU toolchain installation
3. QEMU installation
4. Python 3.12 setup
5. Pytest installation
6. Clean firmware build
7. ELF existence and size verification
8. QEMU integration testing

The CI workflow is:

```text
.github/workflows/ci.yml
```

The intended verification flow is:

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
Runtime Verification
```

This provides automated validation of both firmware compilation and target-level runtime behaviour.

---

## 🔬 FPGA / SPI Simulation

The repository also contains a Verilator-based SPI simulation environment.

Build and run the simulation:

```bash
cd sim
make -f Makefile.sim
./obj_dir/sim_executable
```

Generated Verilator build artifacts are intentionally excluded from Git.

---

## v2.6 Runtime Verification Baseline

The ARM-MSV-Framework-RTOS has reached a reproducible **v2.6 runtime verification baseline** targeting the ARM Cortex-M3 / LM3S6965 platform under QEMU.

### Verified runtime capabilities

* FreeRTOS-based multitasking on ARM Cortex-M3
* Deterministic QEMU-based firmware boot and CLI testing
* Automated Pytest regression suite
* CLI observability and runtime diagnostics
* Task state, priority and stack high-water-mark monitoring
* Heap memory and system uptime reporting
* Watchdog supervision and recovery testing
* HardFault / fault-recovery diagnostics
* GNSS/NMEA parser integration
* TinyML inference task integration
* Thread-safe UART transmit path
* Reproducible firmware build and regression workflow

The current regression baseline has previously reached:

```text
17 passed
```

### Interrupt-driven UART RX

The latest development step introduces an interrupt-driven UART receive path for the LM3S6965 PL011-compatible UART:

```text
UART RX FIFO
     │
     ▼
 UART IRQ5
     │
     ▼
UART0_IRQHandler()
     │
     ▼
Software RX ring buffer
     │
     ▼
uart_getc()
     │
     ▼
FreeRTOS CLI
```

The implementation includes:

* UART0 IRQ5 vector integration
* Cortex-M3 NVIC interrupt enable
* RX and receive-timeout interrupt handling
* Hardware FIFO draining from the ISR
* 256-byte power-of-two software RX ring buffer
* Single-producer / single-consumer RX design
* Non-blocking `uart_getc()` API
* No FreeRTOS API calls from the UART ISR
* Preservation of the existing thread-safe UART TX path

### Current verification status

The interrupt-driven UART RX implementation is integrated on `main` and has been exercised under QEMU.

The existing runtime verification suite remains the reference regression baseline. Additional work is in progress to close the remaining QEMU/Telnet command-stream synchronization issue observed in the full CLI regression after the UART RX architecture change.

This is intentionally tracked as a verification item rather than masked as a passing result.

### Engineering focus

The project is being developed around production-oriented embedded software practices:

* hardware-oriented register-level driver development
* interrupt-driven I/O
* RTOS task scheduling
* watchdog-based fault supervision
* fault and recovery diagnostics
* automated runtime regression
* deterministic simulation
* observability and health monitoring
* reproducible builds
* incremental verification and regression closure

The next development stage focuses on completing interrupt-driven UART RX regression closure and extending the platform toward broader peripheral and system-level verification.
