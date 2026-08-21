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
        pytest.fail(f"firmware.elf not found. Run 'make' first.")

    print("\n[TEST] Launching QEMU (ARM Cortex-M3)...")
    proc = subprocess.Popen([
        "qemu-system-arm", "-M", "lm3s6965evb", 
        "-nographic", "-serial", "telnet:127.0.0.1:4444,server,nowait",
        "-kernel", FIRMWARE_PATH
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    # Δίνουμε χρόνο στο QEMU να ξεκινήσει και στο firmware να κάνει boot.
    time.sleep(3.5) 
    yield proc
    
    proc.terminate()
    proc.wait()

def send_command(cmd, timeout=5.0):
    """
    Στέλνει μια εντολή στο CLI και επιστρέφει την έξοδο μέχρι το επόμενο προτροπή.
    Διαβάζει με timeout και ψάχνει το προτροπή στο συσσωρευμένο data.
    """
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        s.connect(("127.0.0.1", 4444))
        
        # Flush τυχόν υπολειπόμενα δεδομένα (π.χ. boot messages)
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
        
        # Στέλνουμε την εντολή
        s.sendall(f"{cmd}\r\n".encode())
        
        # Διαβάζουμε μέχρι να δούμε το προτροπή στο συσσωρευμένο data
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
                    s.close()
                    return data.decode(errors='ignore')
            except socket.timeout:
                break
        
        s.close()
        # Αν δεν βρέθηκε prompt, επιστρέφουμε ό,τι διαβάστηκε
        return f"TIMEOUT or incomplete. Received: {data.decode(errors='ignore')}"
    except Exception as e:
        return f"Error: {e}"

# --- TESTS ---

def test_shell_help(qemu):
    out = send_command("help")
    assert "predict" in out.lower(), f"Expected 'predict' in help, got: {out}"

def test_tinyml_inference(qemu):
    out = send_command("predict")
    assert "[TinyML]" in out
    assert "Predicted" in out

def test_uptime_counter(qemu):
    out1 = send_command("uptime")
    time.sleep(1.2)
    out2 = send_command("uptime")
    assert out1 != out2

def test_invalid_command(qemu):
    out = send_command("random123")
    assert "Unknown command" in out