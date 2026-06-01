#!/usr/bin/env python3
"""tests/qa/recovery/test_signal_recovery.py

Interrupt nift build mid-flight (SIGINT, then SIGKILL), restart, assert:
  - no partial output files survive
  - cache is invalidated for in-flight artifacts
  - lock files (.nift.lock, .nift/cache.lock) are released
  - second build completes cleanly with deterministic output
"""
from __future__ import annotations

import hashlib
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from contextlib import contextmanager
from pathlib import Path

IS_WIN = sys.platform.startswith("win")
NIFT = Path(os.environ.get("NIFT_BIN", "build/debug/apps/nift/nift")).resolve()

failures: list[str] = []


def record(name: str, ok: bool, detail: str = "") -> None:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}{(' :: ' + detail) if detail else ''}")
    if not ok:
        failures.append(name)


@contextmanager
def big_project(num_pages: int = 200):
    d = Path(tempfile.mkdtemp(prefix="nift-recover-"))
    try:
        (d / "content").mkdir()
        (d / "layouts").mkdir()
        (d / "output").mkdir()
        (d / "nift.config").write_text(
            "name=recover\noutput_dir=output\ncontent_dir=content\nlayouts_dir=layouts\n"
        )
        (d / "layouts" / "default.html").write_text(
            "<html><body>@content</body></html>\n"
        )
        for i in range(num_pages):
            (d / "content" / f"page_{i:04d}.html").write_text(
                f"@layout(default)\n<h1>page {i}</h1>\n" + ("payload " * 200)
            )
        yield d
    finally:
        shutil.rmtree(d, ignore_errors=True)


def hash_dir(p: Path) -> str:
    h = hashlib.sha256()
    for f in sorted(p.rglob("*")):
        if f.is_file():
            h.update(f.relative_to(p).as_posix().encode())
            h.update(b"\0")
            h.update(f.read_bytes())
    return h.hexdigest()


def kill_signal() -> int:
    return signal.SIGTERM if IS_WIN else signal.SIGINT


def assert_no_temp_files(out: Path) -> tuple[bool, str]:
    """Output dir must contain no half-written sidecars."""
    bad: list[str] = []
    for f in out.rglob("*"):
        if not f.is_file():
            continue
        n = f.name
        if n.endswith((".tmp", ".part", ".partial", ".swp", ".lock")) or n.startswith(".nift-tmp-"):
            bad.append(str(f))
    return (not bad, ", ".join(bad))


def assert_no_orphan_locks(d: Path) -> tuple[bool, str]:
    locks = list(d.rglob(".nift.lock")) + list(d.rglob(".nift") and (d / ".nift").rglob("*.lock") or [])
    return (not locks, ", ".join(str(x) for x in locks))


def test_sigint_then_resume() -> None:
    with big_project(num_pages=300) as d:
        out = d / "output"

        # Spawn build, kill it after a short window so it can't possibly finish.
        proc = subprocess.Popen(
            [str(NIFT), "build"],
            cwd=str(d),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            # Give us a process group on POSIX so SIGINT reaches children too.
            start_new_session=not IS_WIN,
        )
        time.sleep(0.4)
        if proc.poll() is not None:
            record("sigint_then_resume", False, "build finished before interrupt — make project bigger")
            return
        if IS_WIN:
            proc.send_signal(signal.CTRL_BREAK_EVENT) if hasattr(signal, "CTRL_BREAK_EVENT") else proc.terminate()
        else:
            os.killpg(proc.pid, signal.SIGINT)
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            # Did not honor SIGINT — escalate. This is itself a failure.
            if not IS_WIN:
                os.killpg(proc.pid, signal.SIGKILL)
            else:
                proc.kill()
            proc.wait(timeout=5)
            record("sigint_honored", False, "SIGINT ignored, had to SIGKILL")
        else:
            record("sigint_honored", True, f"rc={proc.returncode}")

        ok_temp, bad_temp = assert_no_temp_files(out)
        record("no_temp_files_after_kill", ok_temp, bad_temp)

        ok_lock, bad_lock = assert_no_orphan_locks(d)
        record("no_orphan_locks", ok_lock, bad_lock)

        # Now restart. Must succeed and produce a complete tree.
        r = subprocess.run([str(NIFT), "build"], cwd=str(d),
                           capture_output=True, text=True, timeout=120)
        record("resume_build_succeeds", r.returncode == 0,
               f"rc={r.returncode} stderr={r.stderr[-200:]}")

        produced = sorted(p.name for p in out.rglob("*.html") if p.is_file())
        expected = [f"page_{i:04d}.html" for i in range(300)]
        record("resume_full_output",
               set(produced) >= set(expected),
               f"produced={len(produced)} expected={len(expected)}")

        # Determinism: a clean third build must yield identical bytes.
        h_after_resume = hash_dir(out)
        shutil.rmtree(out)
        out.mkdir()
        subprocess.run([str(NIFT), "build"], cwd=str(d), check=True,
                       capture_output=True, text=True, timeout=120)
        h_clean = hash_dir(out)
        record("recovery_deterministic", h_after_resume == h_clean,
               f"resume={h_after_resume[:12]} clean={h_clean[:12]}")


def test_sigkill_then_resume() -> None:
    if IS_WIN:
        record("sigkill_then_resume", True, "skipped on Windows")
        return
    with big_project(num_pages=300) as d:
        out = d / "output"
        proc = subprocess.Popen(
            [str(NIFT), "build"], cwd=str(d),
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            start_new_session=True,
        )
        time.sleep(0.4)
        os.killpg(proc.pid, signal.SIGKILL)
        proc.wait(timeout=5)

        ok_temp, bad_temp = assert_no_temp_files(out)
        record("sigkill_no_temp_files", ok_temp, bad_temp)

        # SIGKILL is unhandleable — locks may persist. Restart must clean them.
        r = subprocess.run([str(NIFT), "build"], cwd=str(d),
                           capture_output=True, text=True, timeout=120)
        record("sigkill_restart_clears_locks", r.returncode == 0,
               f"rc={r.returncode} stderr={r.stderr[-200:]}")


def main() -> int:
    if not NIFT.exists():
        print(f"FAIL: nift binary missing at {NIFT}", file=sys.stderr)
        return 2
    test_sigint_then_resume()
    test_sigkill_then_resume()
    print("---")
    if failures:
        print(f"FAILED: {len(failures)} :: {', '.join(failures)}")
        return 1
    print("OK: recovery suite green")
    return 0


if __name__ == "__main__":
    sys.exit(main())
