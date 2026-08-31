import re
import socket
import time


HOST = "127.0.0.1"
PORT = 4444

BMS_PERIOD_SECONDS = 5.0
MIN_INTERVAL_SECONDS = 4.0
MAX_INTERVAL_SECONDS = 6.0


def test_bms_task_period(qemu):
    """REQ-042: verify periodic BMS task execution in QEMU."""

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(0.5)

    try:
        # The qemu fixture already starts the firmware.
        # Retry because the Telnet server may still be coming up.
        deadline = time.monotonic() + 5.0

        while True:
            try:
                sock.connect((HOST, PORT))
                break
            except OSError:
                if time.monotonic() >= deadline:
                    raise AssertionError(
                        "Could not connect to QEMU Telnet port."
                    )
                time.sleep(0.1)

        data = b""
        bms_timestamps = []
        start = time.monotonic()

        while time.monotonic() - start < 20.0:
            try:
                chunk = sock.recv(4096)

                if not chunk:
                    break

                data += chunk

                while b"\n" in data:
                    line, data = data.split(b"\n", 1)
                    text = line.decode(errors="ignore")

                    if "[BMS] Update:" in text:
                        timestamp = time.monotonic()
                        bms_timestamps.append(timestamp)
                        print(
                            f"[BMS TIMING] update "
                            f"{len(bms_timestamps)} at "
                            f"{timestamp - start:.3f}s: {text.strip()}"
                        )

                        if len(bms_timestamps) >= 4:
                            break

            except socket.timeout:
                continue

            if len(bms_timestamps) >= 4:
                break

        assert len(bms_timestamps) >= 4, (
            f"Expected at least 4 BMS updates, "
            f"got {len(bms_timestamps)}"
        )

        intervals = [
            bms_timestamps[i] - bms_timestamps[i - 1]
            for i in range(1, len(bms_timestamps))
        ]

        print(
            "[BMS TIMING] intervals: "
            + ", ".join(f"{interval:.3f}s" for interval in intervals)
        )

        for index, interval in enumerate(intervals, start=1):
            assert MIN_INTERVAL_SECONDS <= interval <= MAX_INTERVAL_SECONDS, (
                f"BMS interval {index} = {interval:.3f}s; "
                f"expected approximately {BMS_PERIOD_SECONDS:.1f}s"
            )

    finally:
        sock.close()
