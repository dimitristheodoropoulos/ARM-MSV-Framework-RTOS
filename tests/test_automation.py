#!/usr/bin/env python3
import time


def test_shell_help(qemu, send_command):
    out = send_command("help")
    assert "predict" in out.lower(), f"Expected 'predict' in help, got: {out}"


def test_tinyml_inference(qemu, send_command):
    out = send_command("predict")
    assert "[TinyML]" in out
    assert "Predicted" in out


def test_uptime_counter(qemu, send_command):
    out1 = send_command("uptime")
    time.sleep(1.2)
    out2 = send_command("uptime")

    assert out1 != out2


def test_invalid_command(qemu, send_command):
    out = send_command("random123")
    assert "Unknown command" in out
