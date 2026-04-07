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
    
    # ΠΕΡΙΜΕΝΟΥΜΕ ΠΕΡΙΣΣΟΤΕΡΟ (3.5s): 
    # Δίνουμε χρόνο στον ESP8266 driver να κάνει timeout και να ηρεμήσει το UART.
    time.sleep(3.5) 
    yield proc
    
    proc.terminate()
    proc.wait()

def send_command(cmd):
    """Στέλνει εντολή και καθαρίζει το buffer περιμένοντας την απάντηση."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(4.0)
        s.connect(("127.0.0.1", 4444))
        
        # 1. FLUSH: Διαβάζουμε όλο το boot "σκουπίδι" μέχρι να δούμε το prompt 'msv>'
        # Αυτό διασφαλίζει ότι η επόμενη εντολή θα ξεκινήσει από καθαρό έδαφος.
        initial_data = b""
        s.setblocking(False)
        time.sleep(0.2)
        while True:
            try:
                chunk = s.recv(1024)
                if not chunk: break
                initial_data += chunk
            except BlockingIOError:
                break
        s.setblocking(True)

        # 2. SEND: Στέλνουμε την εντολή
        s.send(f"{cmd}\r\n".encode())
        
        # 3. READ: Περιμένουμε την απάντηση
        time.sleep(0.8) 
        data = s.recv(4096)
        s.close()
        
        return data.decode(errors='ignore')
    except Exception as e:
        return f"Error: {e}"

# --- TESTS ---

def test_shell_help(qemu):
    """Επιβεβαιώνει ότι το Help menu περιλαμβάνει το predict"""
    out = send_command("help")
    # Χρησιμοποιούμε assert σε μικρά γράμματα για μέγιστη συμβατότητα
    assert "predict" in out.lower(), f"Expected 'predict' in help menu, got: {out}"

def test_tinyml_inference(qemu):
    """Ελέγχει την AI πρόβλεψη"""
    out = send_command("predict")
    assert "[TinyML]" in out
    assert "Predicted" in out

def test_uptime_counter(qemu):
    """Ελέγχει αν ο SysTick μετράει"""
    out1 = send_command("uptime")
    time.sleep(1.2)
    out2 = send_command("uptime")
    assert out1 != out2

def test_invalid_command(qemu):
    """Ελέγχει το error handling"""
    out = send_command("random123")
    assert "Unknown command" in out