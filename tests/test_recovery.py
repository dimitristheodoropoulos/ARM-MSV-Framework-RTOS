#!/usr/bin/env python3
import pytest


def test_watchdog_recovery(recovery):
    result = recovery.trigger("freeze")

    assert "[TEST] Freezing CLI..." in result.output
    assert result.recovery_status is not None
    assert "WATCHDOG RECOVERY" in result.recovery_status
    assert result.cause is not None
    assert "Task Hang (Heartbeat Timeout)" in result.cause
    assert result.reset_count is not None
    assert result.reset_count >= 1
    assert "rtos_msv>" in result.output


def test_hardfault_recovery(recovery):
    result = recovery.trigger("crash")

    assert "[TEST] Forcing Usage Fault..." in result.output
    assert result.recovery_status is not None
    assert "HARD FAULT RECOVERY" in result.recovery_status
    assert result.cause is not None
    assert "CPU HardFault (Invalid Access)" in result.cause
    assert result.reset_count is not None
    assert result.reset_count >= 1
    assert "rtos_msv>" in result.output