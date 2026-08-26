# BMS Software Architecture

**Document:** `docs/bms_architecture.md`
**Project:** ARM-MSV-Framework-RTOS
**Feature:** BMS-Oriented Embedded Software Foundation
**Status:** Architecture Baseline
**Version:** 1.0
**Target Platform:** ARM Cortex-M3 / FreeRTOS

---

## 1. Purpose

This document defines the software architecture for the BMS-oriented embedded software extension of the ARM-MSV-Framework-RTOS project.

The architecture translates the requirements defined in:

```text
docs/bms_requirements.md
```

into a modular software structure that separates:

* BMS domain logic;
* measurement acquisition;
* protection evaluation;
* fault management;
* state management;
* diagnostics;
* communication;
* hardware abstraction;
* RTOS integration.

The architecture is designed to support deterministic embedded execution, automated testing, simulation and future hardware integration.

---

# 2. Architectural Goals

The BMS architecture shall provide:

1. Clear separation of responsibilities.
2. Minimal coupling between BMS logic and hardware drivers.
3. Independent unit testing of BMS core logic.
4. Deterministic protection evaluation.
5. Explicit BMS state transitions.
6. Centralized fault representation.
7. Hardware-independent measurement interfaces.
8. Hardware-independent communication interfaces.
9. FreeRTOS integration without making the core BMS logic RTOS-dependent.
10. Traceability between requirements, implementation and verification.

---

# 3. Architectural Constraints

The initial implementation targets:

```text
CPU:
ARM Cortex-M3

RTOS:
FreeRTOS

Language:
C

Existing platform:
ARM-MSV-Framework-RTOS

Simulation:
QEMU / host-based verification where applicable

Testing:
Pytest / C test infrastructure as applicable
```

The architecture shall not require physical battery hardware for verification of the core BMS logic.

Physical hardware validation is outside the initial scope.

---

# 4. High-Level Architecture

The BMS software is divided into five logical layers:

```text
┌─────────────────────────────────────────────────────┐
│                 Application / RTOS                  │
│                                                     │
│                 BMS FreeRTOS Task                   │
└────────────────────────┬────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────┐
│                    BMS Core                          │
│                                                     │
│  Orchestration / coordination of BMS processing     │
└────────────────────────┬────────────────────────────┘
                         │
          ┌──────────────┼──────────────┐
          │              │              │
          ▼              ▼              ▼
┌────────────────┐ ┌──────────────┐ ┌───────────────┐
│ Measurements   │ │ Protection   │ │ State Manager │
│                │ │ / Limits     │ │               │
└───────┬────────┘ └──────┬───────┘ └───────┬───────┘
        │                 │                 │
        └─────────────────┼─────────────────┘
                          ▼
                 ┌────────────────┐
                 │ Fault Manager  │
                 └───────┬────────┘
                         │
             ┌───────────┴───────────┐
             ▼                       ▼
      ┌──────────────┐       ┌────────────────┐
      │ Diagnostics  │       │ Communication  │
      │              │       │ CAN / UART     │
      └──────────────┘       └───────┬────────┘
                                     │
                                     ▼
                           Hardware Abstraction
                                     │
                         ┌───────────┴───────────┐
                         ▼                       ▼
                        I²C                     CAN
```

---

# 5. Layer Responsibilities

## 5.1 Application / RTOS Layer

The RTOS/application layer owns:

* task creation;
* task scheduling;
* periodic execution;
* synchronization where required;
* integration with the existing FreeRTOS application.

The RTOS layer shall not contain battery protection algorithms.

### Responsibility boundary

```text
RTOS layer
    ↓
when BMS processing runs

BMS Core
    ↓
what BMS processing does
```

This separation allows BMS core logic to be unit-tested without running FreeRTOS.

---

# 6. BMS Core Layer

The BMS Core is the central coordinator of the BMS processing cycle.

Its responsibility is orchestration rather than implementing every protection rule.

Conceptually:

```text
BMS Core
   │
   ├── acquire measurements
   │
   ├── validate measurements
   │
   ├── evaluate protection
   │
   ├── update faults
   │
   ├── update BMS state
   │
   └── publish diagnostics/status
```

The BMS Core shall not directly access hardware registers.

---

# 7. Measurement Layer

## 7.1 Responsibility

The measurement layer provides normalized BMS measurement data to the BMS core.

Initial measurements:

```text
pack voltage
pack current
battery temperature
```

The measurement layer is responsible for:

* acquiring measurements;
* validating measurements;
* representing measurement status;
* propagating acquisition errors.

---

## 7.2 Measurement Data Model

Conceptually:

```c
typedef enum
{
    BMS_MEAS_VALID,
    BMS_MEAS_INVALID,
    BMS_MEAS_NOT_AVAILABLE,
    BMS_MEAS_OUT_OF_RANGE
} bms_measurement_status_t;
```

A measurement shall carry both:

```text
value
status
```

This prevents the protection layer from treating an invalid measurement as a valid numeric value.

---

## 7.3 Measurement Interface

The BMS core shall interact with measurements through a logical interface such as:

```c
int bms_measurements_read(bms_measurements_t *measurements);
```

The exact API may be refined during implementation.

The interface shall remain independent of the underlying hardware device.

---

# 8. Hardware Measurement Abstraction

The measurement subsystem shall separate:

```text
BMS measurement logic
        │
        ▼
Measurement abstraction
        │
        ▼
I²C / other hardware interface
```

This allows a simulated measurement backend to be used during unit and integration testing.

Example conceptual interface:

```c
int battery_monitor_read_voltage(float *voltage);
int battery_monitor_read_current(float *current);
int battery_monitor_read_temperature(float *temperature);
```

No specific battery-monitor IC is assumed by the architecture.

---

# 9. Protection Layer

## 9.1 Responsibility

The protection layer evaluates validated measurements against configured limits.

Initial protection categories:

```text
over-voltage
under-voltage
over-current
over-temperature
under-temperature
sensor fault
```

Protection evaluation shall be deterministic.

---

## 9.2 Protection Flow

```text
Measurement
     │
     ▼
Validation
     │
     ├── INVALID ──────→ SENSOR FAULT
     │
     ▼
Protection Evaluation
     │
     ├── Voltage
     ├── Current
     └── Temperature
             │
             ▼
         Fault Result
```

---

## 9.3 Protection API

Conceptually:

```c
uint32_t bms_protection_evaluate(
    const bms_measurements_t *measurements,
    const bms_limits_t *limits
);
```

The protection layer shall not directly modify the global BMS state.

It shall produce an explicit result which is consumed by the fault/state-management layers.

---

# 10. Limits Layer

The limits subsystem owns configured protection thresholds.

Example:

```c
typedef struct
{
    float voltage_min;
    float voltage_max;

    float current_max;

    float temperature_min;
    float temperature_max;
} bms_limits_t;
```

The actual representation may be refined during implementation.

The limits layer is responsible for:

* storing limits;
* validating limits;
* providing limits to protection evaluation.

Protection algorithms shall not embed arbitrary threshold constants throughout the code.

---

# 11. Fault Manager

## 11.1 Responsibility

The Fault Manager owns the representation and lifecycle of detected faults.

The initial fault model is:

```c
BMS_FAULT_NONE
BMS_FAULT_OVERVOLTAGE
BMS_FAULT_UNDERVOLTAGE
BMS_FAULT_OVERCURRENT
BMS_FAULT_OVERTEMP
BMS_FAULT_SENSOR
```

Faults shall be represented using a bitmask where simultaneous faults are required.

---

## 11.2 Fault Flow

```text
Protection Result
       │
       ▼
 Fault Manager
       │
       ├── set active fault
       ├── maintain active fault
       ├── classify severity
       └── clear recoverable fault
```

---

## 11.3 Fault Ownership

The Fault Manager owns:

```text
fault state
fault flags
fault lifecycle
fault classification
```

The Protection layer owns:

```text
detection logic
```

This prevents protection algorithms from becoming responsible for global system state.

---

# 12. BMS State Machine

## 12.1 Initial States

The initial BMS state machine shall contain:

```c
BMS_STATE_INIT
BMS_STATE_IDLE
BMS_STATE_MONITORING
BMS_STATE_FAULT
BMS_STATE_RECOVERY
```

---

## 12.2 State Diagram

```text
                    ┌─────────────┐
                    │    INIT     │
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐
                    │    IDLE     │
                    └──────┬──────┘
                           │
                           ▼
                 ┌──────────────────┐
                 │    MONITORING    │
                 └──────┬─────┬─────┘
                        │     │
                  fault │     │ normal
                        │     │
                        ▼     │
                 ┌──────────┐ │
                 │  FAULT   │ │
                 └────┬─────┘ │
                      │        │
              recoverable      │
                      │        │
                      ▼        │
                 ┌──────────┐  │
                 │ RECOVERY │  │
                 └────┬─────┘  │
                      │        │
                      └────────┘
```

The exact transition rules shall be implemented and tested explicitly.

---

# 13. State Transition Rules

Initial conceptual transition rules:

| Current State | Condition               | Next State |
| ------------- | ----------------------- | ---------- |
| INIT          | initialization complete | IDLE       |
| IDLE          | monitoring enabled      | MONITORING |
| MONITORING    | critical fault          | FAULT      |
| MONITORING    | no critical fault       | MONITORING |
| FAULT         | recovery permitted      | RECOVERY   |
| RECOVERY      | recovery successful     | MONITORING |
| RECOVERY      | recovery failed         | FAULT      |

No implicit state transitions shall exist.

---

# 14. Diagnostics Layer

The diagnostics layer exposes internal BMS information without owning the underlying BMS logic.

It shall provide access to:

```text
current BMS state
active fault flags
measurement status
selected measurement values
```

The existing UART/CLI infrastructure may be used as the initial diagnostic transport.

Example conceptual commands:

```text
bms status
bms faults
bms measurements
```

These commands are implementation candidates rather than current verified functionality.

---

# 15. Communication Layer

The communication layer separates BMS data from transport-specific mechanisms.

Initial transports:

```text
UART
CAN
```

The BMS core shall not directly construct hardware-specific CAN registers.

---

# 16. CAN Abstraction

## 16.1 Responsibility

The CAN subsystem provides software-level:

* frame representation;
* frame encoding;
* frame decoding;
* validation;
* error handling.

Conceptual structure:

```c
typedef struct
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} can_frame_t;
```

The exact CAN API shall be determined during implementation.

---

## 16.2 BMS CAN Data

Initial BMS status data candidates:

```text
battery voltage
battery current
battery temperature
BMS state
fault flags
```

The project shall initially verify frame-level software behavior.

Physical CAN bus validation requires separate hardware testing and shall not be implied by software simulation results.

---

# 17. UART Diagnostic Interface

UART remains primarily a diagnostic interface.

Conceptually:

```text
BMS Core
    │
    ▼
Diagnostics
    │
    ▼
UART / CLI
```

The BMS logic shall not depend on UART being available.

This allows the same BMS core to operate in:

```text
QEMU
host tests
target firmware
```

---

# 18. FreeRTOS Integration

The BMS functionality shall be integrated into FreeRTOS through a dedicated task or equivalent application-level scheduling mechanism.

Conceptually:

```c
void bms_task(void *argument)
{
    for (;;)
    {
        bms_process();

        vTaskDelay(...);
    }
}
```

The exact implementation shall be determined after the BMS core API is established.

The following rule applies:

```text
FreeRTOS scheduling
        ≠
BMS protection logic
```

The BMS core must remain callable from a non-RTOS test environment.

---

# 19. BMS Processing Cycle

The intended processing sequence is:

```text
┌───────────────────────────┐
│ Start BMS processing      │
└─────────────┬─────────────┘
              ▼
┌───────────────────────────┐
│ Acquire measurements      │
└─────────────┬─────────────┘
              ▼
┌───────────────────────────┐
│ Validate measurements     │
└─────────────┬─────────────┘
              ▼
┌───────────────────────────┐
│ Evaluate protection       │
└─────────────┬─────────────┘
              ▼
┌───────────────────────────┐
│ Update fault manager      │
└─────────────┬─────────────┘
              ▼
┌───────────────────────────┐
│ Update BMS state          │
└─────────────┬─────────────┘
              ▼
┌───────────────────────────┐
│ Update diagnostics        │
└─────────────┬─────────────┘
              ▼
┌───────────────────────────┐
│ Publish communication     │
└───────────────────────────┘
```

This sequence shall be the primary integration model.

---

# 20. Error Handling

The architecture shall follow a layered error-handling model.

```text
Hardware/Transport Error
          │
          ▼
Measurement / Communication Layer
          │
          ▼
Normalized Error
          │
          ▼
BMS Core
          │
          ▼
Fault Manager
          │
          ▼
BMS State
```

Hardware-specific error codes shall not leak unnecessarily into BMS domain logic.

---

# 21. Dependency Rules

The following dependency direction is required:

```text
Application / RTOS
        ↓
BMS Core
        ↓
BMS Domain Modules
        ↓
Hardware Abstraction
        ↓
Hardware Drivers
```

The following dependency is prohibited:

```text
BMS Protection
       ↓
FreeRTOS API
```

and:

```text
BMS Protection
       ↓
ARM hardware registers
```

The BMS domain shall remain as hardware-independent as reasonably possible.

---

# 22. Proposed Source Tree

The initial BMS source structure is:

```text
src/
├── bms/
│   ├── bms_core.c
│   ├── bms_core.h
│   │
│   ├── bms_measurements.c
│   ├── bms_measurements.h
│   │
│   ├── bms_limits.c
│   ├── bms_limits.h
│   │
│   ├── bms_protection.c
│   ├── bms_protection.h
│   │
│   ├── bms_faults.c
│   ├── bms_faults.h
│   │
│   ├── bms_state.c
│   ├── bms_state.h
│   │
│   ├── bms_diagnostics.c
│   └── bms_diagnostics.h
│
├── drivers/
│   ├── uart.c
│   ├── uart.h
│   ├── i2c.c
│   ├── i2c.h
│   ├── spi.c
│   ├── spi.h
│   └── can.c
│       can.h
│
└── ...
```

The tree represents the intended architecture. Files shall be added incrementally as corresponding requirements are implemented.

---

# 23. Test Architecture

Testing shall be performed at multiple levels.

## 23.1 Unit Testing

Unit tests shall exercise individual BMS modules independently.

Example:

```text
bms_limits
bms_protection
bms_faults
bms_state
bms_measurements
```

Unit tests shall not require:

```text
FreeRTOS scheduler
ARM hardware
physical battery
physical CAN bus
```

where avoidable.

---

## 23.2 Integration Testing

Integration tests shall validate interaction between modules.

Primary integration chain:

```text
Measurements
     ↓
Validation
     ↓
Protection
     ↓
Fault Manager
     ↓
State Machine
```

---

## 23.3 System / Simulation Testing

The BMS application shall be integrated into the existing ARM firmware environment where practical.

Simulation may use:

```text
QEMU
host-based execution
automated regression scripts
```

depending on the capability of each test.

---

# 24. Testability Architecture

The architecture intentionally supports dependency substitution.

For example:

```text
Production:

BMS Core
   ↓
Measurement Interface
   ↓
I²C Driver
   ↓
Hardware
```

Testing:

```text
BMS Core
   ↓
Measurement Interface
   ↓
Simulated Measurement Backend
```

This allows deterministic fault injection.

Example:

```text
inject:
voltage = 60.0 V

expect:
BMS_FAULT_OVERVOLTAGE
```

---

# 25. Fault Injection Strategy

The test environment shall support controlled injection of conditions such as:

```text
over-voltage
under-voltage
over-current
over-temperature
under-temperature
invalid measurement
measurement communication failure
multiple simultaneous faults
```

This allows protection behavior to be tested without physical battery hardware.

---

# 26. Requirement Traceability

The architecture maps requirements to major components as follows:

| Requirements      | Architecture Component           |
| ----------------- | -------------------------------- |
| BMS-REQ-001 – 005 | Measurement Layer                |
| BMS-REQ-006 – 014 | Protection / Limits              |
| BMS-REQ-015 – 019 | Fault Manager                    |
| BMS-REQ-020 – 025 | State Manager                    |
| BMS-REQ-026 – 028 | Limits Layer                     |
| BMS-REQ-029 – 031 | Diagnostics                      |
| BMS-REQ-032 – 037 | Communication / CAN              |
| BMS-REQ-038 – 040 | Measurement Hardware Abstraction |
| BMS-REQ-041 – 043 | RTOS Integration                 |
| BMS-REQ-044 – 046 | BMS Domain Layer                 |
| BMS-REQ-047 – 052 | Verification Architecture        |
| BMS-REQ-053 – 055 | Traceability Process             |
| BMS-REQ-056 – 060 | Software Quality                 |
| BMS-REQ-061 – 064 | Cross-cutting Architecture       |

---

# 27. First Implementation Vertical Slice

The first implementation shall intentionally be limited.

The initial vertical slice shall cover:

```text
BMS-REQ-001
BMS-REQ-004
BMS-REQ-006
BMS-REQ-007
BMS-REQ-015
BMS-REQ-022
```

Functional path:

```text
Voltage Measurement
       ↓
Measurement Validation
       ↓
Voltage Protection
       ↓
Fault Manager
       ↓
BMS State
```

The objective is to establish the complete engineering workflow before expanding the system.

---

# 28. Initial Module Implementation Order

Implementation shall proceed in the following order:

```text
1. bms_measurements
2. bms_limits
3. bms_protection
4. bms_faults
5. bms_state
6. bms_core
7. bms_diagnostics
8. I²C measurement abstraction
9. CAN abstraction
10. FreeRTOS integration
```

Each stage shall be accompanied by tests before moving to the next major layer.

---

# 29. API Design Principles

Public APIs shall:

* use explicit types;
* validate pointers where applicable;
* return explicit status/error values;
* avoid hidden global state where possible;
* avoid direct hardware dependencies;
* expose only required functionality.

Example:

```c
int bms_protection_evaluate(
    const bms_measurements_t *measurements,
    const bms_limits_t *limits,
    uint32_t *faults
);
```

The exact API is subject to implementation review.

---

# 30. Global State Policy

Global mutable state shall be minimized.

Where persistent BMS state is required, it shall be encapsulated within explicit BMS data structures.

Preferred:

```c
bms_context_t
```

containing:

```text
current state
measurements
active faults
limits
diagnostic status
```

rather than unrelated module-level globals.

The final structure shall be determined during implementation.

---

# 31. Configuration Policy

Protection thresholds shall be represented as configuration data rather than scattered compile-time constants.

Initial configuration shall be static and deterministic.

Dynamic configuration, non-volatile storage and field reconfiguration are outside the first implementation scope.

---

# 32. Timing Model

The initial BMS implementation shall use periodic execution.

Conceptually:

```text
BMS task
   │
   ├── execute BMS cycle
   │
   └── delay until next cycle
```

The actual task period shall be selected during FreeRTOS integration and documented as an implementation parameter.

The initial architecture does not claim a production battery-control timing requirement.

---

# 33. Concurrency Model

The initial BMS design shall minimize concurrency.

The preferred model is:

```text
Single BMS processing context
```

with communication and diagnostic interfaces accessing well-defined snapshots or APIs.

RTOS synchronization primitives shall only be introduced where required.

This reduces race-condition risk and simplifies deterministic testing.

---

# 34. Safety-Oriented Design Considerations

The architecture uses several practices relevant to safety-oriented embedded development:

* explicit fault states;
* deterministic transitions;
* input validation;
* separation of protection logic;
* explicit limits;
* defensive error handling;
* traceable requirements;
* automated testing;
* fault injection.

These practices do **not** constitute functional-safety certification.

The project shall not claim IEC 61508 or ISO 26262 compliance without a dedicated compliance process and corresponding evidence.

---

# 35. Verification Evidence

Verification evidence shall be generated from actual execution.

Examples include:

```text
unit-test results
integration-test results
regression logs
coverage reports
static-analysis reports
QEMU execution logs
```

Architecture documentation alone shall not be treated as verification evidence.

---

# 36. Definition of Architectural Completion

The architecture shall be considered implemented when:

* [ ] BMS module boundaries are established.
* [ ] Public interfaces are defined.
* [ ] Measurement abstraction exists.
* [ ] Protection logic is separated from measurement acquisition.
* [ ] Fault management is separated from protection evaluation.
* [ ] State management is explicit.
* [ ] Diagnostics are separated from core logic.
* [ ] Communication is separated from BMS domain logic.
* [ ] FreeRTOS integration does not contaminate core BMS logic.
* [ ] Unit-test seams are available.
* [ ] Integration-test seams are available.
* [ ] Requirement traceability is established.

---

# 37. Architecture Change Policy

Architectural changes shall be made when implementation or verification reveals a genuine design problem.

Changes shall not be introduced merely to increase project size.

When a significant architectural change is made:

1. Update this document.
2. Update affected requirements if necessary.
3. Update the traceability mapping.
4. Add or update tests.
5. Document the reason for the change.

---

# 38. Current Architecture Status

At the creation of this architecture baseline:

```text
Requirements baseline:
Defined

Architecture baseline:
Defined

Implementation:
Pending

Unit verification:
Pending

Integration verification:
Pending

Hardware validation:
Not performed
```

No implementation or verification result shall be inferred from this architecture document.

---

# 39. Engineering Principle

The BMS extension shall follow:

```text
Requirement
     ↓
Architecture
     ↓
Interface
     ↓
Implementation
     ↓
Unit Test
     ↓
Integration Test
     ↓
Regression Evidence
```

The objective is not to maximize the amount of code.

The objective is to produce a small, modular and demonstrably verified embedded-software architecture that can be extended toward realistic BMS functionality without compromising testability or traceability.
