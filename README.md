# ARM MSV OS: High-Reliability RTOS Framework + Edge AI

A production-oriented embedded systems framework for **ARM Cortex-M3 (LM3S6965)**, combining FreeRTOS-based real-time execution, bare-metal drivers, TinyML inference, watchdog supervision, fault handling, and automated QEMU integration testing.

The **v2.5 production baseline** focuses on reproducible builds, automated verification, CLI observability, and a clean CI workflow.

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
- Verilator-based SPI slave simulation.
- Automated GitHub Actions build and integration-test pipeline.

---

## 📂 Project Architecture

```text
ARM-MSV-Framework-RTOS/
├── .github/
│   └── workflows/
│       └── ci.yml              # Build and QEMU integration CI
├── docs/                       # Hardware and protocol documentation
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
│   └── test_cli.py             # CLI integration tests
├── linker.ld                   # Firmware linker script
├── Makefile                    # Firmware build system
├── setup_rtos.sh               # RTOS setup helper
└── README.md
````

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
| `mem`      | Display current and minimum-ever free heap                 |
| `uptime`   | Display system execution time                              |
| `predict`  | Run/display TinyML temperature prediction                  |
| `boost_ai` | Raise AI task priority to level 4                          |
| `low_ai`   | Lower AI task priority to level 1                          |
| `freeze`   | Intentionally suspend the CLI task for watchdog testing    |
| `crash`    | Intentionally trigger a fault for HardFault testing        |

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

The production test suite uses pytest together with QEMU.

Run the complete integration suite:

```bash
pytest -q
```

The current v2.5 baseline contains:

```text
12 tests
```

The tests cover:

* CLI help
* Memory reporting
* Task reporting
* System uptime
* TinyML prediction
* AI priority boost
* AI priority reduction
* Invalid-command handling

The shared QEMU fixture and command transport are implemented in:

```text
tests/conftest.py
```

---

## 🔄 Continuous Integration

GitHub Actions automatically performs:

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

This ensures that every push and pull request is validated through both **firmware compilation** and **runtime integration tests**.

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

## 📊 v2.5 Production Baseline

The current baseline has been locally validated with:

```text
Firmware build:          PASS
QEMU integration tests:  12/12 PASS
Git working tree:        CLEAN
```

The firmware build currently reports approximately:

```text
text    data    bss     dec     hex
26072   84      10644   36800   8fc0
```

These values are build-output measurements for the current development baseline and may change as the firmware evolves.

---

## 🎯 Engineering Goals

The project is intended to demonstrate production-oriented embedded engineering practices, including:

* deterministic embedded execution
* RTOS-based task management
* low-level driver development
* watchdog-based recovery
* fault handling
* lightweight edge inference
* automated system verification
* emulated target testing
* reproducible firmware builds
* continuous integration

The architecture is particularly relevant to **automotive, industrial, robotics, edge-computing, and safety-oriented embedded systems**.

---

## 📌 Project Status

**Current branch:** `feature/v2.5-production-baseline`

**Current milestone:** v2.5 Production Baseline

The baseline currently establishes a clean firmware build, automated QEMU integration testing, and CI verification. Further production hardening can be added incrementally without destabilizing the validated baseline.
