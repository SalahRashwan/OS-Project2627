# Test results — Nostos Phase 1

All results below were run on **26 September 2026**, after the corrections for the
independent audit (`PHASE1_AUDIT_RESULTS.md`). Each one was actually executed and its
output inspected. Results from the earlier 21 September session are superseded by this
file. **Montserrat was not used** (see §10).

## 1. Platform and toolchain

WSL2 Ubuntu 24.04 — a real Linux kernel and userspace, but not the grading host.

```
kernel:   6.18.33.2-microsoft-standard-WSL2 (x86_64)
gcc:      gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
valgrind: valgrind-3.22.0
python3:  3.12 (test helpers only)
lib/sphragis.o SHA-256 identical to Phase1/Sphragis libray/sphragis.o (b50cdd8dfebe3f17...)
```

## 2. Build

```sh
make clean && make all
```

All three executables build with **no gcc warnings** (`-std=gnu11 -Wall -Wextra`).
Building on the Windows-mounted `/mnt/c/...` path makes `make` itself print
`Clock skew detected` (a timestamp quirk of that mount, not a compiler message); in the
fresh-extraction build on native `/tmp` (§9) the warning count was 0.

## 3. Style scan: `python3 tests/style_check.py` (first step of `make test`)

```
style_check: 0 finding(s)
```

Checked mechanically over `src/*.c`, `include/*.h`, `tests/*.c`: a definition header
above every function with `@Name` matching the function, `@Def`, `@Arg` containing
`name = meaning` for every parameter, and `@Ret`; at most 45 lines from signature to
closing brace; no `#include <...>` in a `.c` file; `@File/@Purpose/@Author/@Date` file
headers; include guards; no `b`-prefixed integers; no comparison with a constant on the
right; no prohibited stdio/stat/system calls.

Control run: the same checker on the audited (pre-correction) copy reports over 200
findings, including all the categories named in audit finding F5.

## 4. Functional suite: `make test`

```
=== Summary: 100 passed, 0 failed ===
```

Contents (every case also requires exit status 0 and empty stderr unless a failure is
expected):

- readiness line with the loaded ship name;
- the official test-sheet positive batch (12 commands incl. mixed case) and the four
  official negative cases;
- case-insensitivity, missing/wrong fixed words, missing and extra arguments for every
  verb, prefix impostors (`MAPS`, `ACCEPTED`, `BUYER`), numeric junk, zero/negative,
  `+2`, `INT_MAX` accepted, `INT_MAX + 1` and 20-digit overflow rejected, leading zeros,
  state-independence (`ACCEPT 999`, `SAIL Atlantis`, `BUY UnknownProduct 3`),
  `BUY MAP 1`/`BUY MAP 2`/`SELL MAP 1`;
- blank and whitespace-only lines, tabs, CRLF, final command without newline (both a
  valid one and a usage error);
- 13 argument/initialization failure cases with exact exit status (1 or 2) and a
  required stderr diagnostic: missing files, truncated stock, missing routes marker,
  malformed route after a valid route, truncated Odysseus config, port out of range;
- SIGINT shutdown with bounded waits: Ithaca and all six islands (exit 0, shutdown
  line, empty stderr, loaded voyage count in the readiness line), and Odysseus with
  stdin held open and a partial line buffered;
- loader field comparisons: `tests/bin/loader_check` output diffed against values
  decoded independently from the files (awk for text, `od` for the binary stock):
  every Odysseus field, Ithaca config plus all 24 voyages (IDs 1..24, every field),
  each island's header and raw routes, filtered routes (see §5), and all 48 stock
  records.

`make test` rebuilds `tests/bin/loader_check` from current sources. Control run with
the checker deleted: `80 passed, 29 failed` (the missing prerequisite and every
dependent comparison fail; nothing is skipped).

## 5. Sphragis filtering (real library)

| Island | Raw candidates | Library result | Surviving routes (endpoint from config) |
| --- | ---: | ---: | --- |
| Aeaea | 5 | 2 | Scheria 172.16.214.24:8620, Thrinacia 172.16.214.25:8621 |
| Aeolia | 5 | 1 | Scheria 172.16.214.24:8620 |
| Ismarus | 5 | 1 | Scheria 172.16.214.24:8620 |
| Ogygia | 5 | 1 | Thrinacia 172.16.214.25:8621 |
| Scheria | 5 | 3 | Aeaea 172.16.214.20:8621, Aeolia 172.16.214.21:8622, Ismarus 172.16.214.22:8623 |
| Thrinacia | 5 | 2 | Aeaea 172.16.214.20:8621, Ogygia 172.16.214.23:8624 |

The suite checks, per island: library result = stored route count = the count the
audit observed independently; each stored route appears among the raw routes with the
same IP and port.

## 6. Valgrind: `make memcheck`

Options: `--leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes`.

```
OK: odysseus_sigint_partial (status 0)
OK: ithaca_sigint (status 0)
OK: island_aeaea_sigint (status 0)
OK: island_scheria_sigint (status 0)
OK: odysseus_eof (status 0)
OK: odysseus_bad_args (status 1)
OK: odysseus_missing_config (status 2)
OK: odysseus_truncated_config (status 2)
OK: ithaca_missing_voyages (status 2)
OK: island_truncated_stock (status 2)
OK: island_bad_second_route (status 2)
=== Memcheck summary: 11 clean, 0 failed ===
```

"Clean" means: expected exit status, `ERROR SUMMARY: 0 errors`,
`in use at exit: 0 bytes in 0 blocks`, and no open descriptor other than 0-2 or one
marked `<inherited from parent>` (in every log the only extra descriptor is fd 3,
Valgrind's own log file, marked inherited). The descriptor parser was checked against a
synthetic log containing a non-inherited descriptor: it reported the failure.

## 7. Fault injection: `make faults`

`tests/bin/*-fault` are the real programs linked with test-only wrappers
(`-Wl,--wrap=malloc,--wrap=realloc,--wrap=vasprintf,--wrap=write`). A calibration run
counts the allocation calls of each scenario; the suite then fails each call index in
turn, and separately fails stdout writes from each index onward.

```
ody_map_nl            26 allocation calls      (MAP + newline)
ody_map_eof           26                       (MAP, no newline: EOF path)
ody_sail              27
ody_buy_status_eof    30                       (two commands, last without newline)
island_aeaea          46                       (SIGINT after readiness)
island_scheria        47
island_bad_second_route  1 case               (malformed route after a stored one)
ithaca               132                       (SIGINT after readiness)
ody_output             6 write-failure points
ithaca_output          3
island_output          5
=== Fault summary: 349 passed, 0 failed (valgrind=1) ===
```

Invariants checked on every run: no death by signal/abort; exit 0 only with complete
normal output, exit 2 only with a stderr diagnostic; never `Unknown command` for a valid
command; Valgrind log clean as in §6.

Control run: the same suite (without Valgrind) against the audited code reports the
audit's double free (`free(): double free detected in tcache 2`, exit 134) at
allocation indices 18, 22, 26, 30 for both islands, silent exit-2 failures, a dropped
final command after exit 0, and ignored stdout write failures — i.e. F1-F4 are all
detected by the new tests.

## 8. Terminal CTRL+C and idle CPU

`python3 tests/pty_ctrl_c.py` runs each program on a pseudo-terminal and types the
interrupt character (0x03), so SIGINT comes from the tty line discipline:

```
OK: odysseus terminal CTRL+C -> exit 0   (partial "STAT" typed first)
OK: ithaca terminal CTRL+C -> exit 0
OK: island terminal CTRL+C -> exit 0
```

Idle CPU: Ithaca, Island (Aeaea), and Odysseus (stdin held open) were started together;
`utime + stime` from `/proc/<pid>/stat` increased by **0 ticks** for each over 3 s. All
three then exited 0 on SIGINT.

## 9. Packaging and fresh extraction

On native `/tmp` (copy of this directory):

```sh
make package GROUP=TEST        # trial name only; the real group number is pending
tar -tf GTEST_F1.tar | wc -l   # 92 entries; the only object file is lib/sphragis.o
mkdir /tmp/pkgx && cd /tmp/pkgx && tar -xf GTEST_F1.tar && cd GTEST_F1
make all                       # 0 warnings
make test                      # 100 passed, 0 failed
make memcheck                  # 11 clean, 0 failed
FAULT_VALGRIND=0 make faults   # 349 passed, 0 failed
python3 tests/pty_ctrl_c.py    # 3 OK
```

The trial archive was not kept. The real archive is `make package GROUP=<n>`.

## 10. Not run / pending

- **Montserrat**: nothing was run there. Before submission run §2-§8 on Montserrat.
- **Sphragis negative-return path** (assumption A19): not reachable with the six
  fixture islands; still untested against the real library.
- **Real `G<group>_F1.tar`**: needs the actual group number.
- **Instructor design validation**: not obtained (A02).
