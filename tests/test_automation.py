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


def test_nmea_command(qemu, send_command):
    out = send_command("nmea")

    assert "[NMEA] Sample sentences:" in out
    assert "[NMEA] Parsed output:" in out


def test_nmea_gga_parsing(qemu, send_command):
    out = send_command("nmea")

    assert "[GGA]" in out
    assert "Time=12:35:19" in out
    assert "Lat=48.11730" in out
    assert "Lon=11.51666" in out
    assert "Alt=545.4m" in out
    assert "Sats=8" in out


def test_nmea_rmc_parsing(qemu, send_command):
    out = send_command("nmea")

    assert "[RMC]" in out
    assert "Date=23/3/1994" in out
    assert "Speed=22.4kn" in out
    assert "Course=84.4deg" in out
