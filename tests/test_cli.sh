#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./c-reef}"

if [ ! -x "$BIN" ]; then
    echo "FAIL: binary not executable: $BIN" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

pass() {
    echo "PASS: $1"
}

fail() {
    echo "FAIL: $1" >&2
    exit 1
}

# 1. --help and -h
for flag in --help -h; do
    "$BIN" "$flag" >"$TMP_DIR/help.out" 2>"$TMP_DIR/help.err"
    rc=$?
    [ "$rc" -eq 0 ] || fail "$flag exited with $rc"
    [ ! -s "$TMP_DIR/help.err" ] || fail "$flag wrote to stderr"
    grep -q "Usage:" "$TMP_DIR/help.out" || fail "$flag missing Usage:"
    grep -q -- "--sound" "$TMP_DIR/help.out" || fail "$flag missing --sound"
    grep -q "NO_COLOR" "$TMP_DIR/help.out" || fail "$flag missing NO_COLOR"
done
pass "--help and -h"

# 2. Invalid CLI flags must exit non-zero
for bad in --invalid-flag -Z; do
    set +e
    "$BIN" "$bad" >"$TMP_DIR/bad.out" 2>"$TMP_DIR/bad.err"
    rc=$?
    set -e
    [ "$rc" -ne 0 ] || fail "invalid flag '$bad' unexpectedly exited 0"
    grep -q "unknown option" "$TMP_DIR/bad.err" || fail "invalid flag '$bad' missing error message"
done
pass "invalid CLI flags return non-zero"

# 3. Non-TTY stdout check
set +e
"$BIN" </dev/null >"$TMP_DIR/notty.out" 2>"$TMP_DIR/notty.err"
rc=$?
set -e
[ "$rc" -ne 0 ] || fail "non-TTY stdout unexpectedly exited 0"
grep -q "not a terminal" "$TMP_DIR/notty.err" || fail "non-TTY stdout missing error message"
pass "non-TTY stdout rejected"

# 4-10. Sized PTY tests (small size guard, NO_COLOR, color, 'q', SIGTERM, SIGINT, SIGWINCH)
python3 - "$BIN" <<'PY'
import errno
import fcntl
import os
import pty
import re
import select
import signal
import struct
import subprocess
import sys
import termios
import time

BIN = sys.argv[1]
SGR_RE = re.compile(rb"\x1b\[[0-9;]*m")


def open_sized_pty(cols, rows):
    master, slave = pty.openpty()
    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
    return master, slave


def spawn_in_pty(args, cols, rows, extra_env=None, remove_env=()):
    master, slave = open_sized_pty(cols, rows)
    env = os.environ.copy()
    for k in remove_env:
        env.pop(k, None)
    if extra_env:
        env.update(extra_env)

    def preexec():
        os.setsid()
        fcntl.ioctl(slave, termios.TIOCSCTTY, 0)

    proc = subprocess.Popen(
        [BIN, *args],
        stdin=slave,
        stdout=slave,
        stderr=slave,
        env=env,
        preexec_fn=preexec,
        close_fds=True,
    )
    return proc, master, slave


def read_until(master, proc, predicate, timeout=5.0):
    buf = bytearray()
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        r, _, _ = select.select([master], [], [], 0.05)
        if r:
            try:
                chunk = os.read(master, 65536)
                if chunk:
                    buf.extend(chunk)
            except OSError as e:
                if e.errno != errno.EIO:
                    raise
        if predicate(bytes(buf)):
            return bytes(buf), True
        if proc.poll() is not None:
            # Drain any trailing buffered bytes after exit.
            while True:
                r2, _, _ = select.select([master], [], [], 0.05)
                if not r2:
                    break
                try:
                    chunk = os.read(master, 65536)
                    if not chunk:
                        break
                    buf.extend(chunk)
                except OSError:
                    break
            return bytes(buf), predicate(bytes(buf))
    return bytes(buf), predicate(bytes(buf))


def drain_after_exit(master, proc, timeout=5.0):
    rc = proc.wait(timeout=timeout)
    buf = bytearray()
    while True:
        r, _, _ = select.select([master], [], [], 0.05)
        if not r:
            break
        try:
            chunk = os.read(master, 65536)
            if not chunk:
                break
            buf.extend(chunk)
        except OSError:
            break
    return rc, bytes(buf)


def assert_termios_restored(slave, label):
    attrs = termios.tcgetattr(slave)
    lflag = attrs[3]
    expected = termios.ECHO | termios.ICANON
    assert (lflag & expected) == expected, f"{label}: termios ECHO|ICANON not restored (lflag={lflag:#x})"


def assert_ansi_restored(output, label):
    for seq in (b"\x1b[?1049h", b"\x1b[?7l", b"\x1b[?25l", b"\x1b[0m", b"\x1b[?7h", b"\x1b[?25h", b"\x1b[?1049l"):
        assert seq in output, f"{label}: missing expected terminal sequence {seq!r}"


# Test 4: <30x12 terminal size guard (29x12 and 30x11)
for cols, rows in ((29, 12), (30, 11)):
    proc, master, slave = spawn_in_pty([], cols, rows, remove_env=("NO_COLOR",))
    try:
        rc, out = drain_after_exit(master, proc)
        assert rc != 0, f"{cols}x{rows} unexpectedly exited 0"
        assert b"terminal too small" in out, f"{cols}x{rows} missing size message: {out!r}"
        assert b"\x1b[?1049h" not in out, f"{cols}x{rows} should not enter alt screen"
    finally:
        os.close(slave)
        os.close(master)
print("PASS: <30x12 terminal size guard (29x12 and 30x11)")

# Test 5: Exact minimum 30x12 runs and exits cleanly on 'q'
proc, master, slave = spawn_in_pty([], 30, 12, remove_env=("NO_COLOR",))
try:
    out1, ok = read_until(master, proc, lambda b: b"\x1b[12;1H" in b)
    assert ok, "30x12 did not render a full frame"
    os.write(master, b"q")
    rc, out2 = drain_after_exit(master, proc)
    assert rc == 0, f"30x12 exited with {rc}"
    full = out1 + out2
    assert_ansi_restored(full, "30x12")
    assert_termios_restored(slave, "30x12")
finally:
    os.close(slave)
    os.close(master)
print("PASS: 30x12 minimum size renders and quits cleanly on 'q'")

# Test 6: NO_COLOR=1 under 100x30 PTY emits no SGR color codes in frames
proc, master, slave = spawn_in_pty([], 100, 30, extra_env={"NO_COLOR": "1"})
try:
    out1, ok = read_until(master, proc, lambda b: b"\x1b[30;1H" in b)
    assert ok, "NO_COLOR=1 did not render 100x30 frame"
    os.write(master, b"Q")
    rc, out2 = drain_after_exit(master, proc)
    assert rc == 0, f"NO_COLOR=1 exited with {rc}"
    full = out1 + out2
    assert_ansi_restored(full, "NO_COLOR=1")
    assert_termios_restored(slave, "NO_COLOR=1")
    before_exit = full.split(b"\x1b[0m\x1b[?7h", 1)[0]
    sgr = SGR_RE.findall(before_exit)
    assert not sgr, f"NO_COLOR=1 emitted unexpected SGR sequences in frame output: {sgr[:5]!r}"
finally:
    os.close(slave)
    os.close(master)
print("PASS: NO_COLOR=1 suppresses frame SGR colors under 100x30 PTY")

# Test 7: Color mode + 'q' exit under 100x30 PTY
proc, master, slave = spawn_in_pty([], 100, 30, remove_env=("NO_COLOR",))
try:
    out1, ok = read_until(master, proc, lambda b: b"\x1b[30;1H" in b)
    assert ok, "color mode did not render 100x30 frame"
    os.write(master, b"q")
    rc, out2 = drain_after_exit(master, proc)
    assert rc == 0, f"color mode 'q' exited with {rc}"
    full = out1 + out2
    assert_ansi_restored(full, "color 'q'")
    assert_termios_restored(slave, "color 'q'")
    before_exit = full.split(b"\x1b[0m\x1b[?7h", 1)[0]
    assert SGR_RE.search(before_exit), "expected color SGR sequences when NO_COLOR is unset"
finally:
    os.close(slave)
    os.close(master)
print("PASS: color mode + 'q' clean exit under 100x30 PTY")

# Test 8: Clean SIGTERM exit under 100x30 PTY (default and --sound)
for args, label in (([], "SIGTERM"), (["--sound"], "SIGTERM --sound")):
    proc, master, slave = spawn_in_pty(args, 100, 30, remove_env=("NO_COLOR",))
    try:
        out1, ok = read_until(master, proc, lambda b: b"\x1b[30;1H" in b)
        assert ok, f"{label}: did not render 100x30 frame"
        proc.send_signal(signal.SIGTERM)
        rc, out2 = drain_after_exit(master, proc)
        assert rc == 0, f"{label}: expected exit 0, got {rc}"
        full = out1 + out2
        assert_ansi_restored(full, label)
        assert_termios_restored(slave, label)
    finally:
        os.close(slave)
        os.close(master)
print("PASS: clean SIGTERM exit under 100x30 PTY (with and without --sound)")

# Test 9: Clean SIGINT exit under 100x30 PTY
proc, master, slave = spawn_in_pty([], 100, 30, remove_env=("NO_COLOR",))
try:
    out1, ok = read_until(master, proc, lambda b: b"\x1b[30;1H" in b)
    assert ok, "SIGINT: did not render 100x30 frame"
    proc.send_signal(signal.SIGINT)
    rc, out2 = drain_after_exit(master, proc)
    assert rc == 0, f"SIGINT: expected exit 0, got {rc}"
    full = out1 + out2
    assert_ansi_restored(full, "SIGINT")
    assert_termios_restored(slave, "SIGINT")
finally:
    os.close(slave)
    os.close(master)
print("PASS: clean SIGINT exit under 100x30 PTY")

# Test 10: SIGWINCH resize below minimum and back above minimum
proc, master, slave = spawn_in_pty([], 100, 30, remove_env=("NO_COLOR",))
try:
    out1, ok = read_until(master, proc, lambda b: b"\x1b[30;1H" in b)
    assert ok, "SIGWINCH: initial render failed"
    fcntl.ioctl(master, termios.TIOCSWINSZ, struct.pack("HHHH", 10, 25, 0, 0))
    out2, ok = read_until(master, proc, lambda b: b"terminal too small" in b)
    assert ok, "SIGWINCH: did not display too-small notice when shrunk to 25x10"
    fcntl.ioctl(master, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 80, 0, 0))
    out3, ok = read_until(master, proc, lambda b: b"\x1b[24;1H" in b)
    assert ok, "SIGWINCH: did not resume rendering when resized back to 80x24"
    os.write(master, b"q")
    rc, out4 = drain_after_exit(master, proc)
    assert rc == 0, f"SIGWINCH: expected exit 0, got {rc}"
    assert_ansi_restored(out1 + out2 + out3 + out4, "SIGWINCH")
    assert_termios_restored(slave, "SIGWINCH")
finally:
    os.close(slave)
    os.close(master)
print("PASS: SIGWINCH resize guard and recovery")
PY

echo "ALL TESTS PASSED"
