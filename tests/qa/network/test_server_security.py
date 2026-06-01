#!/usr/bin/env python3
"""tests/qa/network/test_server_security.py

Exercise the dev server (cpp-httplib backed) for path traversal and basic
DoS resilience. The server MUST:
  - 200 only for files inside the served output dir
  - 4xx for any traversal attempt (raw, encoded, double-encoded, mixed)
  - reject absurdly long URLs / headers
  - survive a concurrent flood without crashing or leaking memory

Usage:
    NIFT_BIN=build/debug/apps/nift/nift python3 tests/qa/network/test_server_security.py
"""
from __future__ import annotations

import concurrent.futures as cf
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib.parse
import urllib.request
import urllib.error
from contextlib import contextmanager
from pathlib import Path

NIFT = Path(os.environ.get("NIFT_BIN", "build/debug/apps/nift/nift")).resolve()
HOST = "127.0.0.1"

failures: list[str] = []


def record(name: str, ok: bool, detail: str = "") -> None:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}{(' :: ' + detail) if detail else ''}")
    if not ok:
        failures.append(name)


def free_port() -> int:
    with socket.socket() as s:
        s.bind((HOST, 0))
        return s.getsockname()[1]


@contextmanager
def running_server():
    d = Path(tempfile.mkdtemp(prefix="nift-net-"))
    try:
        (d / "content").mkdir()
        (d / "layouts").mkdir()
        (d / "output").mkdir()
        (d / "nift.config").write_text(
            "name=net\noutput_dir=output\ncontent_dir=content\nlayouts_dir=layouts\n"
        )
        (d / "layouts" / "default.html").write_text("<html>@content</html>\n")
        (d / "content" / "index.html").write_text("@layout(default)\n<p>net</p>\n")

        # Pre-build so server has something to serve.
        subprocess.run([str(NIFT), "build"], cwd=str(d), check=True,
                       capture_output=True, text=True, timeout=60)

        # Plant a "secret" outside the served dir to test traversal.
        secret = d / "secret.txt"
        secret.write_text("TOPSECRET\n")

        port = free_port()
        proc = subprocess.Popen(
            [str(NIFT), "serve", "--host", HOST, "--port", str(port)],
            cwd=str(d),
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True,
        )

        # Wait for socket.
        deadline = time.time() + 10
        while time.time() < deadline:
            try:
                with socket.create_connection((HOST, port), timeout=0.5):
                    break
            except OSError:
                time.sleep(0.1)
        else:
            stderr = proc.stderr.read() if proc.stderr else ""
            proc.kill()
            raise RuntimeError(f"server did not bind: {stderr}")

        try:
            yield d, port
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
    finally:
        shutil.rmtree(d, ignore_errors=True)


def get(port: int, path: str, timeout: float = 5.0) -> tuple[int, bytes]:
    url = f"http://{HOST}:{port}{path}"
    req = urllib.request.Request(url, headers={"User-Agent": "nift-qa/1.0"})
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.status, r.read()
    except urllib.error.HTTPError as e:
        return e.code, e.read() if e.fp else b""
    except Exception as e:  # connection reset, etc.
        return -1, str(e).encode()


# Raw-socket helper so we can send malformed requests urllib refuses to build.
def raw_request(port: int, raw: bytes, timeout: float = 5.0) -> bytes:
    with socket.create_connection((HOST, port), timeout=timeout) as s:
        s.sendall(raw)
        s.settimeout(timeout)
        chunks = []
        try:
            while True:
                buf = s.recv(4096)
                if not buf:
                    break
                chunks.append(buf)
                if sum(len(c) for c in chunks) > 1 << 20:
                    break
        except socket.timeout:
            pass
        return b"".join(chunks)


TRAVERSAL_PAYLOADS = [
    "/../secret.txt",
    "/../../secret.txt",
    "/static/../../secret.txt",
    "/%2e%2e/secret.txt",
    "/%2e%2e%2fsecret.txt",
    "/%252e%252e/secret.txt",      # double-url-encoded
    "/..%2fsecret.txt",
    "/..%5csecret.txt",            # backslash variant for Windows
    "/%c0%ae%c0%ae/secret.txt",    # overlong UTF-8
    "/static/%2e%2e/%2e%2e/secret.txt",
    "/./../secret.txt",
    "/index.html/../../secret.txt",
    # POSIX absolute paths to sensitive system files.
    "/etc/passwd",
    "/proc/self/environ",
    # Windows.
    "/C:/Windows/System32/drivers/etc/hosts",
    "/%5C..%5C..%5Csecret.txt",
]


def test_traversal_blocked(port: int) -> None:
    for payload in TRAVERSAL_PAYLOADS:
        status, body = get(port, payload)
        leaked = b"TOPSECRET" in body
        # Acceptable: 400/403/404 with no leak. 200 with leak == fail.
        ok = (not leaked) and status in (400, 403, 404)
        record(f"traversal::{payload}", ok, f"status={status} leaked={leaked}")


def test_legitimate_request(port: int) -> None:
    status, body = get(port, "/index.html")
    record("legit_index_html", status == 200 and b"<p>net</p>" in body, f"status={status}")
    status, body = get(port, "/")
    record("legit_root", status in (200, 301, 302), f"status={status}")


def test_oversized_url(port: int) -> None:
    long_path = "/" + ("A" * 16384)
    status, _ = get(port, long_path)
    # Must reject; must not crash (next request still works).
    record("oversized_url_rejected", status in (-1, 400, 414, 404), f"status={status}")
    status2, body2 = get(port, "/index.html")
    record("server_alive_after_long_url", status2 == 200, f"status={status2}")


def test_malformed_request(port: int) -> None:
    raw = b"GET /index.html\r\nNoVersion\r\n\r\n"
    resp = raw_request(port, raw)
    record("malformed_request_handled",
           resp.startswith(b"HTTP/1.") and b" 400" in resp[:32] or resp == b"",
           f"resp={resp[:64]!r}")
    # Server still alive.
    status, _ = get(port, "/index.html")
    record("server_alive_after_malformed", status == 200, f"status={status}")


def test_concurrent_flood(port: int) -> None:
    N = 500
    def hit(_):
        s, _b = get(port, "/index.html", timeout=10.0)
        return s
    with cf.ThreadPoolExecutor(max_workers=64) as pool:
        results = list(pool.map(hit, range(N)))
    ok_count = sum(1 for r in results if r == 200)
    # 99% must succeed; the rest can be transient ECONNRESET under load.
    record("flood_500_concurrent", ok_count >= int(N * 0.99),
           f"{ok_count}/{N} 200s")


def test_method_restrictions(port: int) -> None:
    # Dev server should not accept PUT/DELETE writes.
    raw = b"PUT /pwned.html HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\n\r\nPWNED"
    resp = raw_request(port, raw)
    ok = b" 200 " not in resp[:32]
    record("put_rejected", ok, f"resp={resp[:64]!r}")


def main() -> int:
    if not NIFT.exists():
        print(f"FAIL: nift binary missing at {NIFT}", file=sys.stderr)
        return 2
    with running_server() as (_, port):
        test_legitimate_request(port)
        test_traversal_blocked(port)
        test_oversized_url(port)
        test_malformed_request(port)
        test_method_restrictions(port)
        test_concurrent_flood(port)
    print("---")
    if failures:
        print(f"FAILED: {len(failures)} :: {', '.join(failures)}")
        return 1
    print("OK: network suite green")
    return 0


if __name__ == "__main__":
    sys.exit(main())
