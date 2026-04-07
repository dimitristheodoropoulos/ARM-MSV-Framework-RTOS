# ARM MSV OS: High-Reliability RTOS Framework + Edge AI
A professional-grade, multi-tasking embedded system developed for **ARM Cortex-M3 (LM3S6965)**. This project showcases a robust hardware/software co-design ecosystem, featuring a custom **Self-Healing Kernel**, **Predictive AI**, and a comprehensive **Post-Mortem Diagnostic System**.

---

## 🚀 Key Features

### 1. Robust RTOS Kernel (FreeRTOS)
* **Preemptive Multitasking**: Coexistence of CLI, AI Inference, and System Monitor tasks with optimized priority levels.
* **Dynamic Task Tuning**: Support for runtime priority adjustments (`boost_ai`/`low_ai`) to demonstrate real-time scheduling control.
* **System Observability**: Live tracking of Task States, CPU utilization (`ps`), and System Uptime.

### 2. High-Reliability & Self-Healing
* **Watchdog Monitor (WDT)**: A dedicated high-priority supervisor task that triggers a hardware reset via the **AIRCR register** upon task hangs or heartbeat timeouts.
* **Post-Mortem Logging**: Implementation of a **Persistent Boot Log** in a non-initialized RAM section (`.noinit`). This survives resets, allowing the system to report the exact cause of failure (WDT Timeout vs. HardFault).
* **Exception Handling**: Custom `HardFault_Handler` in Assembly/C for trapping illegal memory accesses and invalid processor states.

### 3. Edge AI (TinyML)
* **Inference Engine**: Pure C implementation of a Linear Regression model for **Predictive Maintenance**.
* **Trend Analysis**: Analyzes sensor data in real-time to predict overheating or mechanical failure before they occur.

### 4. Bare-Metal Driver Suite
* **Register-Level Drivers**: UART, GPIO, SysTick, I2C, and SPI developed without vendor HALs to ensure maximum performance and minimal footprint.
* **Memory Management**: Integrated **Heap_4** allocation monitoring via the CLI (`mem` command) to track memory leaks and watermarks.

### 5. Verification & Emulation
* **QEMU Integration**: Full system emulation with interactive Telnet/Serial support.
* **FPGA Co-Simulation**: Verilog SPI Slave BFM translated via **Verilator** for cycle-accurate hardware-in-the-loop testing.
* **Automated Pytest Suite**: Black-box validation of CLI commands and AI accuracy over TCP sockets.

---

## 📂 Project Architecture

```text
ARM_MSV_Framework/
├── src/
│   ├── drivers/        # Register-level UART, GPIO, I2C, SPI
│   ├── kernel/         # Shell CLI, TinyML Engine, RTOS Hooks
│   ├── gnss/           # NMEA Parser (GGA/RMC)
│   └── main.c          # Kernel Init, Task Creation, Fault Handlers
├── scripts/            # ML Training (Python) & Automated Pytest Suite
├── sim/                # Verilator C++ testbench & Verilog models
└── Makefile            # Master build system (GNU Toolchain)

🛠️ Tech Stack & Tools
Languages: C (Embedded), ARM Assembly, Verilog, Python (ML & Test Automation).

OS/RTOS: FreeRTOS.

Toolchain: arm-none-eabi-gcc, GNU Make.

Simulation: QEMU (Cortex-M3), Verilator.

💻 System Commands (CLI)
The interactive shell (rtos_msv>) provides deep system insights:

Command,Description
ps,"Display Task States, Priorities, and Stack High Watermarks."
mem,Show Current Free Heap and Lifetime Minimum Free Memory.
uptime,Show system execution time (minutes/seconds).
boost_ai,Dynamically elevate AI Task priority to Level 4.
freeze,Simulate a Task Hang to verify Watchdog Recovery.
crash,Trigger an Invalid State Fault to test HardFault Handling.

⚙️ How to Build & Run
1. Compile and Launch (QEMU)
make clean && make
qemu-system-arm -M lm3s6965evb -nographic -kernel firmware.elf

2. Run Automated Verification
In a separate terminal:
pytest scripts/test_automation.py

3. FPGA SPI Simulation
cd sim && make -f Makefile.sim && ./obj_dir/sim_executable

📉 Real-World Impact
Automotive/Industrial: Self-healing ECUs that log crash data and recover automatically from transient faults.

Edge Computing: Real-time sensor analysis without the need for cloud connectivity.

Mission Critical: Guaranteed task execution through strict priority scheduling and watchdog supervision.

Developed by Dimitris - Embedded Systems & Hardware Verification Engineer