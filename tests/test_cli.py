#!/usr/bin/env python3
import time


def test_help_menu(qemu, send_command):
    out = send_command("help")

    assert "predict" in out
    assert "mem" in out
    assert "ps" in out
    assert "uptime" in out
    assert "boost_ai" in out
    assert "low_ai" in out
    assert "freeze" in out
    assert "crash" in out


def test_mem_command(qemu, send_command):
    out = send_command("mem")

    assert "Free now:" in out
    assert "Lifetime min:" in out


def test_ps_command(qemu, send_command):
    out = send_command("ps")

    assert "CLI" in out
    assert "AI" in out
    assert "WDT" in out


def test_uptime_command(qemu, send_command):
    out1 = send_command("uptime")
    time.sleep(1.2)
    out2 = send_command("uptime")

    assert out1 != out2


def test_predict_command(qemu, send_command):
    out = send_command("predict")

    assert "[TinyML]" in out
    assert "Predicted" in out


def test_boost_ai(qemu, send_command):
    out = send_command("boost_ai")

    assert "AI Priority boosted to 4" in out


def test_low_ai(qemu, send_command):
    out = send_command("low_ai")

    assert "AI Priority lowered to 1" in out


def test_invalid_command(qemu, send_command):
    out = send_command("random123")

    assert "Unknown command" in out
