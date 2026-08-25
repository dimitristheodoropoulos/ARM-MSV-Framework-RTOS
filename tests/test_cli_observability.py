import socket
import subprocess
import time
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = PROJECT_ROOT / "firmware.elf"

HOST = "127.0.0.1"
PORT = 4444


def recv_until(sock, marker, timeout=10):
    sock.settimeout(0.5)
    data = b""
    deadline = time.time() + timeout

    while time.time() < deadline:
        try:
            chunk = sock.recv(4096)

            if not chunk:
                break

            data += chunk

            if marker.encode() in data:
                break

        except socket.timeout:
            continue

    return data.decode(errors="replace")


def drain_socket(sock):
    """Discard stale Telnet/QEMU bytes before issuing a new command."""
    sock.settimeout(0.05)

    while True:
        try:
            chunk = sock.recv(4096)
            if not chunk:
                break
        except socket.timeout:
            break


def parse_stack_hwm(output):
    """Parse the STACK HIGH-WATER MARK table into {task: hwm}."""
    values = {}

    for line in output.splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[0] in {"CLI", "IDLE", "AI", "WDT"}:
            try:
                values[parts[0]] = int(parts[1])
            except ValueError:
                pass

    return values


def send_command(sock, command, timeout=5):
    # Remove stale prompt/echo bytes before issuing the next command.
    drain_socket(sock)

    sock.sendall((command + "\r\n").encode())

    # Wait until the command echo has appeared.
    data = b""
    deadline = time.time() + timeout
    sock.settimeout(0.5)

    while time.time() < deadline:
        try:
            chunk = sock.recv(4096)

            if not chunk:
                break

            data += chunk

            text = data.decode(errors="replace")

            # We only accept a prompt that appears AFTER the
            # command echo.
            echo_pos = text.find(command)

            if echo_pos >= 0:
                prompt_pos = text.find("rtos_msv> ", echo_pos + len(command))

                if prompt_pos >= 0:
                    return text

        except socket.timeout:
            continue

    return data.decode(errors="replace")



def main():
    print("[1/8] Checking firmware...")
    if not FIRMWARE.exists():
        raise SystemExit("FAIL: firmware.elf not found. Run 'make' first.")

    print("[2/8] Starting QEMU...")

    qemu = subprocess.Popen(
        [
            "qemu-system-arm",
            "-M", "lm3s6965evb",
            "-nographic",
            "-serial", f"telnet:{HOST}:{PORT},server,nowait",
            "-kernel", str(FIRMWARE),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    try:
        print("[3/8] Waiting for Telnet server...")

        sock = None

        for _ in range(50):
            try:
                sock = socket.create_connection((HOST, PORT), timeout=1)
                break
            except OSError:
                time.sleep(0.2)

        if sock is None:
            raise SystemExit("FAIL: Could not connect to QEMU Telnet port.")

        with sock:

            print("[4/8] Waiting for boot/prompt...")

            boot = recv_until(sock, "rtos_msv> ", timeout=10)

            print("--- BOOT OUTPUT ---")
            print(boot)

            print("[5/8] Idle watchdog verification...")
            time.sleep(25)

            idle_probe = send_command(sock, "ps")

            print("--- IDLE PROBE ---")
            print(idle_probe)

            full_output = boot + idle_probe

            if "Task Hang" in full_output:
                raise AssertionError(
                    "FAIL: Watchdog reported Task Hang during CLI idle."
                )

            if "[KERNEL] EMERGENCY RESET" in full_output:
                raise AssertionError(
                    "FAIL: Emergency reset detected during CLI idle."
                )

            print("PASS: CLI remained alive during idle UART wait.")

            print("[6/8] Running observability commands...")

            ps1 = idle_probe
            mem = send_command(sock, "mem")
            uptime = send_command(sock, "uptime")

            print("--- PS ---")
            print(ps1)

            print("--- MEM ---")
            print(mem)

            print("--- UPTIME ---")
            print(uptime)

            if "CLI" not in ps1 or "AI" not in ps1 or "WDT" not in ps1:
                raise AssertionError(
                    "FAIL: Expected RTOS tasks missing from ps output."
                )

            if "Free now:" not in mem:
                raise AssertionError(
                    "FAIL: mem command did not return heap information."
                )

            if "System Uptime:" not in uptime:
                raise AssertionError(
                    "FAIL: uptime command did not return uptime."
                )

            stack = send_command(sock, "stack")

            print("--- STACK ---")
            print(stack)

            hwm = parse_stack_hwm(stack)
            required_tasks = {"CLI", "IDLE", "AI", "WDT"}

            if set(hwm) != required_tasks:
                raise AssertionError(
                    f"FAIL: incomplete stack HWM data: {hwm!r}"
                )

            for task_name, value in hwm.items():
                if value <= 0:
                    raise AssertionError(
                        f"FAIL: invalid stack HWM for {task_name}: {value}"
                    )

            print(f"PASS: stack HWM values valid: {hwm}")

            print("[7/8] Verifying AI priority transitions...")

            boost = send_command(sock, "boost_ai")
            ps_boost = send_command(sock, "ps")

            print("--- BOOST ---")
            print(boost)

            print("--- PS AFTER BOOST ---")
            print(ps_boost)

            if "[SYSTEM] AI Priority boosted to 4" not in boost:
                raise AssertionError(
                    "FAIL: boost_ai did not report priority 4."
                )

            def get_ai_priority(ps_output):
                for line in ps_output.splitlines():
                    fields = line.split()
                    if len(fields) >= 5 and fields[0] == "AI":
                        try:
                            return int(fields[2])
                        except ValueError:
                            return None
                return None

            ai_priority_boost = get_ai_priority(ps_boost)

            if ai_priority_boost != 4:
                raise AssertionError(
                    f"FAIL: expected AI priority 4 after boost, "
                    f"got {ai_priority_boost!r}.\\n"
                    f"PS output:\\n{ps_boost}"
                )

            low = send_command(sock, "low_ai")
            ps_low = send_command(sock, "ps")

            print("--- LOW ---")
            print(low)

            print("--- PS AFTER LOW ---")
            print(ps_low)

            if "[SYSTEM] AI Priority lowered to 1" not in low:
                raise AssertionError(
                    "FAIL: low_ai did not report priority 1."
                )

            ai_priority_low = get_ai_priority(ps_low)

            if ai_priority_low != 1:
                raise AssertionError(
                    f"FAIL: expected AI priority 1 after low_ai, "
                    f"got {ai_priority_low!r}.\\n"
                    f"PS output:\\n{ps_low}"
                )

            full_output += mem + uptime + boost + ps_boost + low + ps_low

            if "Task Hang" in full_output:
                raise AssertionError(
                    "FAIL: Watchdog Task Hang detected during test."
                )

            if "[KERNEL] EMERGENCY RESET" in full_output:
                raise AssertionError(
                    "FAIL: Emergency reset detected during test."
                )

            print("[8/8] Final result")
            print("========================================")
            print("PASS: CLI observability regression test")
            print("========================================")

    finally:
        print("Stopping QEMU...")
        qemu.terminate()

        try:
            qemu.wait(timeout=3)
        except subprocess.TimeoutExpired:
            qemu.kill()
            qemu.wait()


if __name__ == "__main__":
    main()
