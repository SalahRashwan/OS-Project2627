#!/usr/bin/env python3
# @File: pty_ctrl_c.py
# @Purpose: Test-only check of a terminal-generated CTRL+C. Runs each
#           program on a pseudo-terminal, waits for its readiness text,
#           types the terminal interrupt character (0x03) so the tty line
#           discipline itself raises SIGINT, and requires exit status 0
#           plus the shutdown text. Odysseus also receives a partial
#           command first, to show a buffered line is discarded.
#           Usage: python3 tests/pty_ctrl_c.py  (from the project root)
#           Exit status: 0 = all passed, 1 = at least one failure.
# @Author: Salah Ahmed Salaheldin Adly Rashwan
# @Date: 2026-09-26
import os
import pty
import select
import sys
import time

CASES = [
    ('odysseus', ['./odysseus', 'configs/odysseus.dat'], b'ready to sail', b'STAT', b''),
    ('ithaca', ['./ithaca', 'configs/ithaca.dat', 'data/voyages.dat'],
     b'Waiting for Odysseus...', b'', b'Ithaca closes the harbor.'),
    ('island', ['./island', 'configs/aeaea.dat', 'data/stocks/Aeaea.db'],
     b'products available.', b'', b'Aeaea closes its port.'),
]


def read_until(fd, text, limit):
    data = b''
    deadline = time.time() + limit
    while text not in data and time.time() < deadline:
        ready, _, _ = select.select([fd], [], [], 0.1)
        if ready:
            try:
                chunk = os.read(fd, 4096)
            except OSError:
                break
            if not chunk:
                break
            data += chunk
    return data


def run_case(name, argv, ready, partial, bye):
    pid, fd = pty.fork()
    if pid == 0:
        os.execv(argv[0], argv)
    output = read_until(fd, ready, 10)
    if partial:
        os.write(fd, partial)
        time.sleep(0.2)
    os.write(fd, b'\x03')
    output += read_until(fd, bye if bye else b'\x00never', 5)
    deadline = time.time() + 5
    status = None
    while time.time() < deadline:
        done, raw = os.waitpid(pid, os.WNOHANG)
        if done:
            status = raw
            break
        time.sleep(0.05)
    os.close(fd)
    if status is None:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
        print(f'FAIL: {name} did not exit after terminal CTRL+C')
        return False
    ok = os.WIFEXITED(status) and os.WEXITSTATUS(status) == 0 and (not bye or bye in output)
    print(f'{"OK" if ok else "FAIL"}: {name} terminal CTRL+C -> '
          f'{"exit " + str(os.WEXITSTATUS(status)) if os.WIFEXITED(status) else "signal"}')
    return ok


def main():
    results = [run_case(*case) for case in CASES]
    return 0 if all(results) else 1


if __name__ == '__main__':
    sys.exit(main())
