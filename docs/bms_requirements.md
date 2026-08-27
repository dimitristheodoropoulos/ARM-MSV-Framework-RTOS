# BMS Software Requirements Specification

**Document:** `docs/bms_requirements.md`
**Project:** ARM-MSV-Framework-RTOS
**Feature:** BMS-Oriented Embedded Software Foundation
**Status:** Draft — Implementation Baseline
**Version:** 1.0
**Target Platform:** ARM Cortex-M3 / FreeRTOS
**Verification Approach:** Unit, Integration, Simulation and Regression Testing

---

## 1. Purpose

This document defines the software requirements for the Battery Management System (BMS)-oriented software extension of the ARM-MSV-Framework-RTOS project.

The objective is to provide a structured embedded-software layer for:

* battery measurement acquisition;
* measurement validation;
* battery protection monitoring;
* fault detection and reporting;
* BMS state management;
* diagnostic information;
* communication abstraction;
* automated software verification.

The implementation is intended as a **BMS-oriented embedded software prototype** running on an ARM Cortex-M3 / FreeRTOS platform.

The project does not claim validation against real battery hardware, a production battery pack, a specific battery-monitor IC, CAN hardware, or functional-safety certification unless explicitly verified in a separate test campaign.

---

# 2. Scope

The BMS software scope includes:

1. Battery measurement data modelling.
2. Measurement validation.
3. Voltage monitoring.
4. Current monitoring.
5. Temperature monitoring.
6. Battery protection limits.
7. Fault detection.
8. Fault classification.
9. BMS state-machine management.
10. Fault recovery handling.
11. Diagnostic status reporting.
12. Communication abstraction.
13. CAN frame abstraction and software-level verification.
14. Unit testing.
15. Integration testing.
16. Simulation/regression testing through the existing project verification infrastructure.

The following are explicitly outside the initial scope:

* physical battery-cell balancing hardware;
* real battery-pack control;
* charger hardware;
* contactor hardware;
* real CAN transceiver validation;
* production automotive certification;
* IEC 61508 certification;
* ISO 26262 certification;
* CANopen conformance;
* proprietary battery-management protocols;
* real-time State-of-Charge estimation algorithms;
* real-time State-of-Health estimation algorithms.

These capabilities may be considered future extensions.

---

# 3. Design Principles

The BMS software shall follow the following engineering principles:

* deterministic embedded behavior;
* explicit state transitions;
* separation of measurement, protection and communication logic;
* testable C modules;
* clear fault ownership;
* defensive input validation;
* traceable requirements;
* automated regression testing;
* reproducible simulation;
* no dependency on physical hardware for core software unit tests.

---

# 4. Requirement Identification

Requirements use the following identifier:

```text
BMS-REQ-NNN
```

where `NNN` is a unique sequential requirement number.

Each requirement is intended to be traceable to:

```text
Requirement
    ↓
Design / Source Module
    ↓
Unit Test
    ↓
Integration Test
    ↓
Verification Result
```

---

# 5. Functional Requirements

## 5.1 Measurement Management

### BMS-REQ-001 — Battery Voltage Measurement

The BMS software shall provide an interface for obtaining the battery pack voltage measurement.

**Priority:** High

**Verification:** Unit test / Integration test

**Planned implementation:**

```text
src/bms/bms_measurements.c
src/bms/bms_measurements.h
```

**Planned test:**

```text
tests/test_bms_measurements.py
```

---

### BMS-REQ-002 — Battery Current Measurement

The BMS software shall provide an interface for obtaining the battery pack current measurement.

**Priority:** High

**Verification:** Unit test / Integration test

**Planned implementation:**

```text
src/bms/bms_measurements.c
```

**Planned test:**

```text
tests/test_bms_measurements.py
```

---

### BMS-REQ-003 — Battery Temperature Measurement

The BMS software shall provide an interface for obtaining the battery temperature measurement.

**Priority:** High

**Verification:** Unit test / Integration test

**Planned implementation:**

```text
src/bms/bms_measurements.c
```

**Planned test:**

```text
tests/test_bms_measurements.py
```

---

### BMS-REQ-004 — Measurement Validity

The BMS software shall validate measurement data before the measurements are used by protection logic.

Invalid measurement data shall not be treated as valid battery measurements.

**Priority:** Critical

**Verification:** Unit test

**Planned implementation:**

```text
src/bms/bms_measurements.c
```

**Test cases shall include:**

* valid measurement;
* invalid measurement;
* out-of-range measurement;
* missing measurement;
* multiple invalid measurements.

---

### BMS-REQ-005 — Measurement Status

The BMS software shall associate a validity/status indication with each battery measurement.

Example conceptual states:

```text
VALID
INVALID
NOT_AVAILABLE
OUT_OF_RANGE
```

**Priority:** High

**Verification:** Unit test

---

# 6. Protection Requirements

## 6.1 Voltage Protection

### BMS-REQ-006 — Over-Voltage Detection

The BMS software shall detect a battery voltage condition exceeding the configured over-voltage protection threshold.

**Priority:** Critical

**Verification:** Unit test / Integration test

**Expected result:**

```text
measurement > configured_limit
        ↓
OVERVOLTAGE fault
```

---

### BMS-REQ-007 — Under-Voltage Detection

The BMS software shall detect a battery voltage condition below the configured under-voltage protection threshold.

**Priority:** Critical

**Verification:** Unit test / Integration test

**Expected result:**

```text
measurement < configured_limit
        ↓
UNDERVOLTAGE fault
```

---

### BMS-REQ-008 — Voltage Boundary Handling

The BMS software shall define deterministic behavior when the measured voltage is exactly equal to a configured protection threshold.

**Priority:** High

**Verification:** Boundary-value unit test

**Purpose:**

This requirement prevents ambiguous behavior at protection boundaries.

---

# 7. Current Protection Requirements

### BMS-REQ-009 — Over-Current Detection

The BMS software shall detect a battery current condition exceeding the configured over-current protection threshold.

**Priority:** Critical

**Verification:** Unit test / Integration test

---

### BMS-REQ-010 — Current Boundary Handling

The BMS software shall define deterministic behavior when measured current is exactly equal to the configured over-current threshold.

**Priority:** High

**Verification:** Boundary-value unit test

---

### BMS-REQ-011 — Invalid Current Protection Input

The BMS software shall treat an invalid current measurement as a diagnostic/protection condition rather than as a valid zero-current measurement.

**Priority:** Critical

**Verification:** Unit test

---

# 8. Temperature Protection Requirements

### BMS-REQ-012 — Over-Temperature Detection

The BMS software shall detect a battery temperature condition exceeding the configured maximum operating temperature threshold.

**Priority:** Critical

**Verification:** Unit test / Integration test

---

### BMS-REQ-013 — Under-Temperature Detection

The BMS software shall detect a battery temperature condition below the configured minimum operating temperature threshold when such a limit is enabled.

**Priority:** High

**Verification:** Unit test

---

### BMS-REQ-014 — Temperature Boundary Handling

The BMS software shall define deterministic behavior when measured temperature is exactly equal to a configured temperature protection threshold.

**Priority:** High

**Verification:** Boundary-value unit test

---

# 9. Fault Management Requirements

### BMS-REQ-015 — Fault Classification

The BMS software shall represent detected protection and diagnostic conditions using explicit fault identifiers.

The initial fault model shall support at least:

```text
BMS_FAULT_NONE
BMS_FAULT_OVERVOLTAGE
BMS_FAULT_UNDERVOLTAGE
BMS_FAULT_OVERCURRENT
BMS_FAULT_OVERTEMP
BMS_FAULT_SENSOR
```

**Priority:** Critical

**Verification:** Unit test

**Planned implementation:**

```text
src/bms/bms_faults.c
src/bms/bms_faults.h
```

---

### BMS-REQ-016 — Multiple Fault Representation

The BMS software shall support representation of multiple simultaneously active fault conditions.

**Priority:** High

**Verification:** Unit test

**Example:**

```text
OVERVOLTAGE
+
OVERTEMP
```

shall be representable simultaneously.

---

### BMS-REQ-017 — Fault Persistence

The BMS software shall maintain the active fault status until the fault is cleared according to the defined fault-management policy.

**Priority:** High

**Verification:** Unit test

---

### BMS-REQ-018 — Fault Clearing

The BMS software shall provide a deterministic mechanism for clearing faults that are no longer active and are permitted to recover.

**Priority:** High

**Verification:** Unit test / Integration test

---

### BMS-REQ-019 — Critical Fault Identification

The BMS software shall distinguish protection-critical faults from non-critical diagnostic conditions.

**Priority:** Critical

**Verification:** Unit test

---

# 10. BMS State Management

### BMS-REQ-020 — Initialization State

The BMS software shall enter an explicit initialization state during startup.

**Priority:** High

**Verification:** Unit test / Integration test

---

### BMS-REQ-021 — Monitoring State

The BMS software shall provide an explicit monitoring state for normal battery supervision.

**Priority:** High

**Verification:** Unit test

---

### BMS-REQ-022 — Fault State

The BMS software shall transition to a fault state when a configured critical protection condition is detected.

**Priority:** Critical

**Verification:** Unit test / Integration test

---

### BMS-REQ-023 — Recovery State

The BMS software shall support an explicit recovery state for conditions where recovery is permitted.

**Priority:** High

**Verification:** Unit test

---

### BMS-REQ-024 — Deterministic State Transitions

BMS state transitions shall be deterministic and shall be based on explicitly defined input conditions.

**Priority:** Critical

**Verification:** Unit test / Integration test

---

### BMS-REQ-025 — Invalid State Protection

The BMS software shall handle invalid or unexpected state values deterministically.

**Priority:** High

**Verification:** Unit test

---

# 11. Protection Limit Configuration

### BMS-REQ-026 — Configurable Protection Limits

The BMS software shall maintain protection limits independently from the protection evaluation logic.

**Priority:** High

**Verification:** Unit test

**Planned implementation:**

```text
src/bms/bms_limits.c
src/bms/bms_limits.h
```

---

### BMS-REQ-027 — Limit Validation

Protection limits shall be validated before being accepted by the BMS protection subsystem.

**Priority:** High

**Verification:** Unit test

---

### BMS-REQ-028 — Invalid Configuration Handling

The BMS software shall reject or safely handle invalid protection-limit configurations.

Examples include:

```text
minimum voltage >= maximum voltage
minimum temperature >= maximum temperature
negative/invalid limits where unsupported
```

**Priority:** Critical

**Verification:** Unit test

---

# 12. Diagnostics

### BMS-REQ-029 — Diagnostic Status

The BMS software shall expose the current BMS state and active fault status through a diagnostic interface.

**Priority:** High

**Verification:** Unit test / Integration test

---

### BMS-REQ-030 — Fault Visibility

Active faults shall be observable through the existing firmware diagnostic infrastructure.

**Priority:** High

**Verification:** Integration test

**Potential interface:**

```text
UART / CLI
```

---

### BMS-REQ-031 — Fault Context

Diagnostic reporting shall provide sufficient information to identify the active fault category.

Where applicable, diagnostic information shall include:

```text
fault identifier
BMS state
measurement status
```

**Priority:** Medium

**Verification:** Integration test

---

# 13. Communication Requirements

## 13.1 Communication Abstraction

### BMS-REQ-032 — Communication Abstraction

The BMS software shall separate BMS data handling from the underlying communication transport.

**Priority:** High

**Verification:** Unit test / Design inspection

---

## 13.2 CAN Software Abstraction

### BMS-REQ-033 — CAN Frame Representation

The BMS software shall provide a software representation of a CAN frame containing at least:

```text
CAN identifier
DLC
data payload
```

**Priority:** High

**Verification:** Unit test

**Planned implementation:**

```text
src/drivers/can.c
src/drivers/can.h
```

or an equivalent BMS communication abstraction.

---

### BMS-REQ-034 — CAN Frame Encoding

The BMS software shall provide software-level encoding of selected BMS status information into CAN payloads.

Initial candidate data:

```text
battery voltage
battery current
battery temperature
BMS state
fault flags
```

**Priority:** High

**Verification:** Unit test

---

### BMS-REQ-035 — CAN Frame Decoding

The BMS software shall provide software-level decoding of supported BMS CAN frames.

**Priority:** Medium

**Verification:** Unit test

---

### BMS-REQ-036 — CAN Error Handling

The communication layer shall provide deterministic handling of invalid or malformed CAN frame data.

**Priority:** High

**Verification:** Unit test

---

### BMS-REQ-037 — CAN Hardware Scope

CAN software verification shall not be interpreted as physical CAN-transceiver or bus validation unless a corresponding hardware test is explicitly performed.

**Priority:** Critical

**Verification:** Documentation / Review

---

# 14. I²C Measurement Abstraction

### BMS-REQ-038 — Measurement Device Abstraction

The BMS software shall provide an abstraction between the BMS measurement layer and a potential I²C-connected measurement device.

**Priority:** High

**Verification:** Unit test / Integration test

---

### BMS-REQ-039 — I²C Error Propagation

Communication errors occurring while obtaining measurements through the I²C abstraction shall be propagated to the BMS measurement layer.

**Priority:** High

**Verification:** Unit test

---

### BMS-REQ-040 — Measurement Communication Failure

A failed measurement transaction shall not be interpreted as a valid battery measurement.

**Priority:** Critical

**Verification:** Unit test / Integration test

---

# 15. RTOS Integration

### BMS-REQ-041 — BMS Task

The BMS software shall be integrable as a FreeRTOS task without requiring the BMS core logic to depend directly on RTOS primitives.

**Priority:** High

**Verification:** Integration test / Design inspection

---

### BMS-REQ-042 — Periodic Monitoring

The BMS monitoring task shall execute periodically according to a configurable scheduling interval.

**Priority:** High

**Verification:** Integration test

---

### BMS-REQ-043 — Separation of Application and RTOS Logic

Core BMS protection and state-management logic shall remain independently testable without starting the FreeRTOS scheduler.

**Priority:** Critical

**Verification:** Unit test / Design inspection

---

# 16. Defensive Software Requirements

### BMS-REQ-044 — Invalid Input Handling

BMS modules shall handle invalid input data deterministically without causing uncontrolled behavior.

**Priority:** Critical

**Verification:** Unit test

---

### BMS-REQ-045 — No Undefined Protection Behavior

Protection evaluation shall produce a defined result for all supported input ranges.

**Priority:** Critical

**Verification:** Boundary-value testing / Unit testing

---

### BMS-REQ-046 — Integer and Floating-Point Safety

Where arithmetic is used for protection evaluation, the implementation shall explicitly consider overflow, underflow, invalid values and numerical boundary conditions relevant to the selected representation.

**Priority:** High

**Verification:** Unit test / Static analysis

---

# 17. Verification Requirements

### BMS-REQ-047 — Unit Test Coverage

Each safety-relevant BMS functional module shall have automated unit tests covering normal, boundary and fault conditions.

**Priority:** Critical

**Verification:** Test report

---

### BMS-REQ-048 — Boundary Testing

Protection thresholds shall be tested at:

```text
below threshold
exactly at threshold
above threshold
```

where applicable.

**Priority:** Critical

**Verification:** Unit test

---

### BMS-REQ-049 — Fault Combination Testing

The test suite shall verify operation when multiple fault conditions are active simultaneously.

**Priority:** High

**Verification:** Unit test

---

### BMS-REQ-050 — Integration Testing

BMS modules shall be tested together to verify interactions between:

```text
measurements
    ↓
validation
    ↓
protection
    ↓
fault manager
    ↓
state machine
```

**Priority:** Critical

**Verification:** Integration test

---

### BMS-REQ-051 — Regression Testing

BMS tests shall be integrated into the existing project regression infrastructure where technically applicable.

**Priority:** Critical

**Verification:** CI / Regression test

---

### BMS-REQ-052 — Simulation Testing

The BMS software shall support software-level verification without requiring physical battery hardware.

**Priority:** High

**Verification:** Simulation / QEMU / Host-based testing

---

# 18. Traceability Requirements

### BMS-REQ-053 — Requirement Traceability

Each implemented BMS requirement shall be traceable to at least one implementation location and one verification test.

**Priority:** Critical

**Verification:** Documentation review

---

### BMS-REQ-054 — Test Traceability

Each BMS test shall identify the requirement(s) it verifies.

Example:

```text
test_bms_overvoltage_detection
    → BMS-REQ-006
```

**Priority:** High

**Verification:** Test review

---

### BMS-REQ-055 — Unimplemented Requirements

Requirements that have not yet been implemented shall be explicitly identified as pending and shall not be represented as verified functionality.

**Priority:** Critical

**Verification:** Documentation review

---

# 19. Software Quality Requirements

### BMS-REQ-056 — Modular C Implementation

The BMS software shall be implemented using modular C components with clearly defined interfaces.

**Priority:** High

**Verification:** Code review

---

### BMS-REQ-057 — Separation of Concerns

Measurement acquisition, protection evaluation, fault management, state management and communication shall be implemented as logically separated modules.

**Priority:** High

**Verification:** Architecture/code review

---

### BMS-REQ-058 — Static Analysis

BMS source code shall be suitable for static-analysis tooling and shall avoid constructs that unnecessarily prevent automated analysis.

**Priority:** High

**Verification:** Static analysis

---

### BMS-REQ-059 — Defensive Coding

BMS modules shall use defensive programming practices appropriate for embedded firmware, including explicit validation of external inputs and deterministic error handling.

**Priority:** High

**Verification:** Code review / Static analysis

---

### BMS-REQ-060 — Reproducible Verification

The BMS verification process shall be executable repeatedly using the project's documented build and test procedures.

**Priority:** Critical

**Verification:** Regression test

---

# 20. Non-Functional Requirements

### BMS-REQ-061 — Determinism

Protection evaluation and state transitions shall execute deterministically for identical input conditions.

**Priority:** Critical

**Verification:** Unit / Integration testing

---

### BMS-REQ-062 — Testability

Core BMS logic shall be testable independently from hardware drivers and RTOS scheduling.

**Priority:** Critical

**Verification:** Unit testing / Architecture review

---

### BMS-REQ-063 — Maintainability

BMS modules shall use clear interfaces and avoid unnecessary coupling between components.

**Priority:** High

**Verification:** Code review

---

### BMS-REQ-064 — Portability

The BMS core logic shall minimize direct dependency on ARM-specific hardware registers.

**Priority:** High

**Verification:** Architecture/code review

---

# 21. Initial Traceability Matrix

| Requirement Range | Planned Module                  | Verification           |
| ----------------- | ------------------------------- | ---------------------- |
| BMS-REQ-001 – 005 | `bms_measurements`              | Unit + Integration     |
| BMS-REQ-006 – 014 | `bms_limits` / `bms_protection` | Unit + Boundary        |
| BMS-REQ-015 – 019 | `bms_faults`                    | Unit                   |
| BMS-REQ-020 – 025 | `bms_state`                     | Unit + Integration     |
| BMS-REQ-026 – 028 | `bms_limits`                    | Unit                   |
| BMS-REQ-029 – 031 | diagnostics                     | Integration            |
| BMS-REQ-032 – 037 | CAN abstraction                 | Unit                   |
| BMS-REQ-038 – 040 | I²C / measurement abstraction   | Unit + Integration     |
| BMS-REQ-041 – 043 | FreeRTOS integration            | Integration            |
| BMS-REQ-044 – 046 | BMS core                        | Unit + Static Analysis |
| BMS-REQ-047 – 052 | Verification infrastructure     | Test/CI                |
| BMS-REQ-053 – 055 | Traceability documentation      | Review                 |
| BMS-REQ-056 – 060 | Software quality                | Review/CI              |
| BMS-REQ-061 – 064 | Architecture/core               | Review + Testing       |

---

# 22. Verification Status

At the creation of this requirements baseline:

```text
Requirements defined:       64
Requirements implemented:    0
Requirements verified:       0
Requirements pending:       64
```

This status shall be updated only after implementation and verification evidence exists.

No requirement shall be marked as verified based solely on design intent.

---

# 23. Future Extensions

Potential future requirements may address:

* State-of-Charge estimation;
* State-of-Health estimation;
* cell-level monitoring;
* cell balancing;
* contactor control;
* charger interaction;
* thermal-management interfaces;
* persistent fault logging;
* CAN message database;
* CANopen integration;
* battery pack simulation;
* hardware-in-the-loop testing;
* formal verification of selected safety-critical state transitions.

These features are intentionally excluded from the initial implementation baseline.

---

# 24. Definition of Done

The BMS software foundation shall be considered complete for this development phase when:

* [ ] BMS requirements are implemented or explicitly marked out-of-scope.
* [ ] BMS architecture documentation exists.
* [ ] Measurement abstraction is implemented.
* [ ] Protection logic is implemented.
* [ ] Fault management is implemented.
* [ ] BMS state machine is implemented.
* [ ] CAN software abstraction is implemented.
* [ ] I²C measurement abstraction is integrated where applicable.
* [ ] Unit tests are implemented.
* [ ] Boundary-value tests are implemented.
* [ ] Fault-combination tests are implemented.
* [ ] Integration tests are implemented.
* [ ] Regression testing passes.
* [ ] Requirement-to-test traceability is complete.
* [ ] No unverified functionality is presented as verified.
* [ ] README documentation reflects only demonstrated functionality.

---

# 25. Traceability Policy

The project shall follow the rule:

```text
Requirement
    ↓
Implementation
    ↓
Test
    ↓
Evidence
    ↓
Verified
```

A feature is considered **implemented** only when source code exists.

A feature is considered **tested** only when an automated or documented test exercises it.

A feature is considered **verified** only when the corresponding test passes and the result is recorded.

This distinction shall be preserved throughout the BMS development process.
