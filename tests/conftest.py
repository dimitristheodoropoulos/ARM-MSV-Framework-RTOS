#!/usr/bin/env python3
import subprocess
import time
import socket
import pytest
import os
import re

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
        "-serial", "telnet:127.0.0.1:4444,server,nowait,nodelay",
        "-kernel", FIRMWARE_PATH
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    time.sleep(3.5)

    yield proc

    proc.terminate()
    proc.wait()


@pytest.fixture
def send_command():
    def _send_command(cmd, timeout=5.0):
        s = None
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(timeout)
            s.connect(("127.0.0.1", 4444))

            # Flush
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

            s.sendall(f"{cmd}\r\n".encode())

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

            return "TIMEOUT or incomplete. Received: " + data.decode(errors="ignore")

        except Exception as e:
            return f"Error: {e}"
        finally:
            if s:
                try:
                    s.close()
                except:
                    pass

    return _send_command


# ---------- RECOVERY HELPER ----------

class RecoveryResult:
    def __init__(self, command, output):
        self.command = command
        self.output = output
        self.recovery_status = None
        self.cause = None
        self.reset_count = None
        self._parse_output()

    def _parse_output(self):
        status_match = re.search(r"STATUS:\s*([A-Z_ ]+)", self.output)
        if status_match:
            self.recovery_status = status_match.group(1).strip()

        cause_match = re.search(r"CAUSE:\s*(.*?)(?:\r\n|\n|$)", self.output)
        if cause_match:
            self.cause = cause_match.group(1).strip()

        reset_match = re.search(r"TOTAL RESETS:\s*(\d+)", self.output)
        if reset_match:
            self.reset_count = int(reset_match.group(1))


class RecoveryHelper:
    def trigger(self, command, timeout=15.0):
        """
        Στέλνει `freeze` ή `crash` και επιστρέφει το πλήρες output
        μέχρι το post-reset CLI prompt.
        """
        s = None
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(timeout)
            s.connect(("127.0.0.1", 4444))

            # Flush
            s.setblocking(False)
            time.sleep(0.2)
            while True:
                try:
                    s.recv(1024)
                except BlockingIOError:
                    break
            s.setblocking(True)

            s.sendall(f"{command}\r\n".encode())

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
                        break
                except socket.timeout:
                    break

            output = data.decode(errors="ignore")
            return RecoveryResult(command, output)

        except Exception as e:
            return RecoveryResult(command, f"Error: {e}")
        finally:
            if s:
                try:
                    s.close()
                except:
                    pass


@pytest.fixture
def recovery(qemu):
    """Fixture for recovery tests (uses the same QEMU instance)."""
    return RecoveryHelper()
