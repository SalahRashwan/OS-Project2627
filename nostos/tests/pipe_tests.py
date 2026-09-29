#!/usr/bin/env python3
"""
@File: pipe_tests.py
@Purpose: Real-pipe regression tests for broken stdout/stderr on all three
          programs. Each case builds real OS pipes, closes every reader
          where the case needs a broken pipe, runs the real executable
          and checks its exit status and stderr. A process killed by a
          signal (for example SIGPIPE, status -13) always fails.
          Children start with the default SIGPIPE action
          (restore_signals=True), and a control case checks that a
          plain `yes` is still killed by SIGPIPE under this harness.
          Usage: python3 tests/pipe_tests.py [--valgrind]
          With --valgrind every case runs under Valgrind (memcheck +
          descriptor tracking) with its log in tests/pipe-logs/.
          Exit status: 0 = all passed, 1 = at least one failure,
          77 = --valgrind asked but valgrind is missing (nothing run).
@Author: Salah Ahmed Salaheldin Adly Rashwan
@Date: 2026-09-29
"""
import os
import re
import select
import shutil
import signal
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_DIR = os.path.join(ROOT, "tests", "pipe-logs")
VALGRIND = ["valgrind", "--leak-check=full", "--show-leak-kinds=all",
            "--track-origins=yes", "--track-fds=yes"]

# argv and the last bytes of the readiness output for each program.
VALID = {
    "odysseus": (["./odysseus", "configs/odysseus.dat"], b"$ "),
    "ithaca": (["./ithaca", "configs/ithaca.dat", "data/voyages.dat"],
               b"Waiting for Odysseus...\n"),
    "island": (["./island", "configs/aeaea.dat", "data/stocks/Aeaea.db"],
               b"products available.\n"),
}
BAD_ARGS = {
    "odysseus": ["./odysseus"],
    "ithaca": ["./ithaca", "configs/ithaca.dat"],
    "island": ["./island", "configs/aeaea.dat"],
}
MISSING_CONFIG = {
    "odysseus": ["./odysseus", "configs/no-such-file.dat"],
    "ithaca": ["./ithaca", "configs/no-such-file.dat", "data/voyages.dat"],
    "island": ["./island", "configs/no-such-file.dat", "data/stocks/Aeaea.db"],
}
# stderr text expected when stdout breaks after a successful startup.
WRITE_ERROR = {
    "odysseus": b"Error: writing to the terminal failed.",
    "ithaca": b"Error: Ithaca could not write to standard output.",
    "island": b"Error: Island could not write to standard output.",
}
SHUTDOWN = {
    "odysseus": None,
    "ithaca": b"Ithaca closes the harbor.\n",
    "island": b"Aeaea closes its port.\n",
}


class Case:
    """One running test child plus the descriptors the harness owns."""

    def __init__(self, name, use_valgrind):
        self.name = name
        self.log = None
        if use_valgrind:
            self.log = os.path.join(LOG_DIR, name + ".log")
        self.timeout = 90 if use_valgrind else 10
        self.err_file = tempfile.TemporaryFile()
        self.proc = None

    def start(self, argv, stdin, stdout, stderr):
        """Starts argv with the given descriptors (None = working stderr)."""
        if self.log is not None:
            argv = VALGRIND + ["--log-file=" + self.log] + argv
        if stderr is None:
            stderr = self.err_file
        self.proc = subprocess.Popen(argv, cwd=ROOT, stdin=stdin, stdout=stdout,
                                     stderr=stderr, close_fds=True,
                                     restore_signals=True)

    def wait(self):
        """Waits (bounded) and returns the status, or None on timeout."""
        try:
            return self.proc.wait(timeout=self.timeout)
        except subprocess.TimeoutExpired:
            self.proc.kill()
            self.proc.wait()
            return None

    def stderr_text(self):
        self.err_file.seek(0)
        return self.err_file.read()

    def close(self):
        if self.proc is not None and self.proc.poll() is None:
            self.proc.kill()
            self.proc.wait()
        self.err_file.close()


def dead_pipe():
    """Returns the write end of a pipe whose only read end is closed."""
    nRead, nWrite = os.pipe()
    os.close(nRead)
    return nWrite


def read_until(fd, marker, timeout):
    """Reads fd until the collected bytes end with marker; None on timeout."""
    data = b""
    deadline = time.monotonic() + timeout
    while not data.endswith(marker):
        left = deadline - time.monotonic()
        if left <= 0:
            return None
        ready, _, _ = select.select([fd], [], [], left)
        if ready:
            chunk = os.read(fd, 4096)
            if not chunk:
                return None
            data += chunk
    return data


def valgrind_problem(log):
    """Returns why a Valgrind log is not clean, or None if it is."""
    try:
        with open(log, "r", errors="replace") as handle:
            text = handle.read()
    except OSError as error:
        return "no Valgrind log (%s)" % error
    if "ERROR SUMMARY: 0 errors" not in text:
        return "Valgrind reported errors"
    if "in use at exit: 0 bytes in 0 blocks" not in text:
        return "memory still in use at exit"
    leaked = []
    fd = None
    inherited = False
    for line in text.splitlines() + ["Open file descriptor -1:"]:
        match = re.search(r"Open (?:file|AF_\w+ socket) descriptor (-?\d+)", line)
        if match:
            if fd is not None and not inherited:
                leaked.append(fd)
            fd = int(match.group(1))
            inherited = 0 <= fd <= 2
        elif "<inherited from parent>" in line:
            inherited = True
    if leaked:
        return "application descriptor(s) left open: %s" % leaked
    return None


def judge(case, status, expected, need_text):
    """Returns None if the finished case passed, else the failure reason."""
    if status is None:
        return "did not finish within %d s (killed)" % case.timeout
    if status < 0:
        return "killed by signal %d (%s)" % (-status, signal.Signals(-status).name)
    if status != expected:
        return "exit status %d, expected %d" % (status, expected)
    err = case.stderr_text()
    if need_text is True and not err.strip():
        return "no diagnostic on stderr"
    if isinstance(need_text, bytes) and need_text not in err:
        return "stderr lacks %r (got %r)" % (need_text, err)
    if case.log is not None:
        return valgrind_problem(case.log)
    return None


def case_dead_stdout_at_start(case, prog):
    """A: stdout is readerless before startup; stdin held open."""
    argv, _ = VALID[prog]
    nIn, nInWrite = os.pipe()
    nOut = dead_pipe()
    case.start(argv, nIn, nOut, None)
    os.close(nIn)
    os.close(nOut)
    status = case.wait()
    os.close(nInWrite)
    return judge(case, status, 2, True)


def case_reader_gone_after_ready(case, prog):
    """B: read readiness, close every reader, then force one more write."""
    argv, marker = VALID[prog]
    nIn, nInWrite = os.pipe()
    nOutRead, nOut = os.pipe()
    case.start(argv, nIn, nOut, None)
    os.close(nIn)
    os.close(nOut)
    ready = read_until(nOutRead, marker, case.timeout)
    os.close(nOutRead)
    if ready is None:
        os.close(nInWrite)
        case.wait()
        return "readiness output never arrived"
    if "odysseus" == prog:
        os.write(nInWrite, b"MAP\n")
    else:
        case.proc.send_signal(signal.SIGINT)
    status = case.wait()
    os.close(nInWrite)
    return judge(case, status, 2, WRITE_ERROR[prog])


def case_sigint_working_stdout(case, prog):
    """B (control): SIGINT with a working stdout still exits 0."""
    argv, marker = VALID[prog]
    nIn, nInWrite = os.pipe()
    nOutRead, nOut = os.pipe()
    case.start(argv, nIn, nOut, None)
    os.close(nIn)
    os.close(nOut)
    ready = read_until(nOutRead, marker, case.timeout)
    if ready is not None:
        case.proc.send_signal(signal.SIGINT)
    status = case.wait()
    rest = b""
    chunk = os.read(nOutRead, 4096)
    while chunk:
        rest += chunk
        chunk = os.read(nOutRead, 4096)
    os.close(nOutRead)
    os.close(nInWrite)
    if ready is None:
        return "readiness output never arrived"
    if SHUTDOWN[prog] is not None and SHUTDOWN[prog] not in rest:
        return "shutdown line missing (got %r)" % rest
    return judge(case, status, 0, False)


def case_both_dead(case, prog):
    """C: stdout and stderr are both readerless; must still exit 2."""
    argv, _ = VALID[prog]
    nIn, nInWrite = os.pipe()
    nOut = dead_pipe()
    nErr = dead_pipe()
    case.start(argv, nIn, nOut, nErr)
    for fd in (nIn, nOut, nErr):
        os.close(fd)
    status = case.wait()
    os.close(nInWrite)
    return judge(case, status, 2, False)


def case_dead_stderr(case, argv, expected):
    """C: wrong arguments or a missing config with a readerless stderr."""
    nErr = dead_pipe()
    case.start(argv, subprocess.DEVNULL, subprocess.DEVNULL, nErr)
    os.close(nErr)
    return judge(case, case.wait(), expected, False)


def build_cases():
    """Returns (name, function, arguments) for every regression case."""
    cases = []
    for prog in ("odysseus", "ithaca", "island"):
        cases.append(("A_dead_stdout_at_start_" + prog, case_dead_stdout_at_start, (prog,)))
        cases.append(("B_reader_gone_after_ready_" + prog, case_reader_gone_after_ready, (prog,)))
        cases.append(("B_sigint_working_stdout_" + prog, case_sigint_working_stdout, (prog,)))
        cases.append(("C_stdout_and_stderr_dead_" + prog, case_both_dead, (prog,)))
        cases.append(("C_bad_args_dead_stderr_" + prog, case_dead_stderr, (BAD_ARGS[prog], 1)))
        cases.append(("C_missing_config_dead_stderr_" + prog, case_dead_stderr,
                      (MISSING_CONFIG[prog], 2)))
    return cases


def harness_control():
    """Checks children really start with default SIGPIPE: `yes` must die."""
    nOut = dead_pipe()
    proc = subprocess.Popen(["yes"], stdout=nOut, stderr=subprocess.DEVNULL,
                            close_fds=True, restore_signals=True)
    os.close(nOut)
    try:
        status = proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
        return "control `yes` did not finish"
    if -signal.SIGPIPE != status:
        return "control `yes` exited %d, not by SIGPIPE: children may inherit SIG_IGN" % status
    return None


def main():
    use_valgrind = "--valgrind" in sys.argv[1:]
    if use_valgrind:
        if shutil.which("valgrind") is None:
            print("NOT RUN: valgrind not found on PATH. No pipe case was checked.")
            return 77
        os.makedirs(LOG_DIR, exist_ok=True)
        for old in os.listdir(LOG_DIR):
            os.remove(os.path.join(LOG_DIR, old))
    for prog in VALID:
        if not os.access(os.path.join(ROOT, prog), os.X_OK):
            print("FAIL: ./%s is not built (run make first)" % prog)
            return 1
    passed = 0
    failed = 0
    problem = harness_control()
    if problem is None:
        print("OK: harness control (`yes` killed by SIGPIPE)")
        passed += 1
    else:
        print("FAIL: harness control -- " + problem)
        failed += 1
    for name, function, args in build_cases():
        case = Case(name, use_valgrind)
        try:
            problem = function(case, *args)
        finally:
            case.close()
        if problem is None:
            print("OK: " + name + (" (log tests/pipe-logs/%s.log)" % name if use_valgrind else ""))
            passed += 1
        else:
            print("FAIL: %s -- %s" % (name, problem))
            failed += 1
    mode = "valgrind" if use_valgrind else "plain"
    print("Pipe tests (%s): %d passed, %d failed" % (mode, passed, failed))
    return 0 if 0 == failed else 1


if __name__ == "__main__":
    sys.exit(main())
