#!/usr/bin/env python3
import subprocess
import time
import socket
import pytest
import os

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
FIRMWARE_PATH = os.path.join(PROJECT_ROOT, "firmware.elf")


@pytest.fixture(scope="module")
def qemu():
    if not os.path.exists(FIRMWARE_PATH):
        pytest.fail("firmware.elf not found. Run 'make' first.")

    print("\n[TEST] Launching QEMU (ARM Cortex-M3)...")

    proc = subprocess.Popen([
        "qemu-system-arm",
        "-M", "lm3s6965evb",
        "-nographic",
        "-serial", "telnet:127.0.0.1:4444,server,nowait",
        "-kernel", FIRMWARE_PATH
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    time.sleep(3.5)

    yield proc

    proc.terminate()
    proc.wait()


@pytest.fixture
def send_command():
    def _send_command(cmd, timeout=5.0):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(timeout)
            s.connect(("127.0.0.1", 4444))

            # Flush boot / previous output
            s.setblocking(False)
            time.sleep(0.2)

            while True:
                try:
                    chunk = s.recv(1024)
                    if not chunk:
                        break
                except BlockingIOError:
                    break

            s.setblocking(True)

            # Send command
            s.sendall(f"{cmd}\r\n".encode())

            # Read until CLI prompt
            data = b""
            prompt = b"rtos_msv> "
            start = time.time()

            while time.time() - start < timeout:
                try:
                    chunk = s.recv(1024)

                    if not chunk:
                        break

                    data += chunk

                    if prompt in data:
                        return data.decode(errors="ignore")

                except socket.timeout:
                    break

            return (
                "TIMEOUT or incomplete. Received: "
                + data.decode(errors="ignore")
            )

        except Exception as e:
            return f"Error: {e}"

        finally:
            try:
                s.close()
            except:
                pass

    return _send_command
