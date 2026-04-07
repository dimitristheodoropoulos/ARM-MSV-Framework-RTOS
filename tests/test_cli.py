import socket
import time
import pytest

@pytest.fixture(scope="module")
def board():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(2.0)
    s.connect(("127.0.0.1", 4444))
    yield s
    s.close()

def send_and_get(s, cmd):
    # Καθαρισμός τυχόν σκουπιδιών από προηγούμενες εντολές
    s.setblocking(False)
    try:
        while s.recv(4096): pass
    except:
        pass
    s.setblocking(True)

    s.sendall(cmd.encode() + b'\r\n')
    time.sleep(0.4) # Λίγο περισσότερο χρόνο για το I2C scan που είναι αργό
    out = ""
    try:
        while True:
            data = s.recv(4096).decode(errors='ignore')
            out += data
            if "msv>" in out: break
    except socket.timeout:
        pass
    return out

def test_system_commands(board):
    # 1. Test Help
    out = send_and_get(board, "help")
    assert "Framework" in out
    
    # 2. Test Uptime
    out = send_and_get(board, "uptime")
    assert any(x in out.lower() for x in ["tick", "ms"])
    
    # 3. Test GNSS
    out = send_and_get(board, "gnss")
    assert "Lat=" in out
    
    # 4. Test I2C (Διορθωμένο Assert)
    out = send_and_get(board, "i2c")
    assert "I2C" in out and "Found" in out 
    
    # 5. Test SPI
    out = send_and_get(board, "spi")
    assert "SPI" in out and "PASS" in out

def test_diagnostics(board):
    out = send_and_get(board, "test")
    assert "PASS" in out