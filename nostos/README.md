# The Nostos System — Phase 1: Inithaca

A course-compliant Phase 1 implementation of the Nostos System (OS 2026-27), covering
Odysseus, Ithaca, and Island process startup, configuration/data loading, mandatory
Sphragis route filtering, and a syntax-only interactive command parser. No sockets,
concurrency mechanisms, trading, or persistence are implemented — see
`docs/assumptions.md` and `PHASE1_IMPLEMENTATION_GUIDE.md` for the full scope rationale.

## Prerequisites

- A GNU/Linux environment (Montserrat is the graded target; this implementation was
  built and tested on WSL2 Ubuntu 24.04 — a genuine GNU/Linux system, not Windows
  emulation — see `docs/test-results.md` for exact versions).
- `gcc` supporting `-std=gnu11` (tested with GCC 13.3.0).
- `valgrind` for the memory/descriptor checks (optional but strongly recommended).

## Build

```sh
make            # builds odysseus, ithaca, island (default target)
make clean      # removes only generated build/ objects and the three executables
```

`island` is the only executable linked against the real `lib/sphragis.o`. A clean build
produces no compiler warnings under `-Wall -Wextra`.

## Run

```sh
./odysseus <config.dat>
./ithaca <config.dat> <voyages.dat>
./island <config.dat> <stock.db>
```

Ready-to-use fixtures are provided:

```sh
./odysseus configs/odysseus.dat
./ithaca configs/ithaca.dat data/voyages.dat
./island configs/aeaea.dat data/stocks/Aeaea.db
```

Any of the six islands can be started by pairing its config with its matching stock
file: `configs/{aeaea,aeolia,ismarus,ogygia,scheria,thrinacia}.dat` with
`data/stocks/{Aeaea,Aeolia,Ismarus,Ogygia,Scheria,Thrinacia}.db`.

Only `odysseus` reads an interactive terminal. `ithaca` and `island` print a startup
message and then block (no busy waiting — verified at 0 CPU ticks over several idle
seconds under `/proc/<pid>/stat`, see `docs/test-results.md`) until **CTRL+C**, at which
point they print a shutdown message, release every owned allocation and descriptor, and
exit with status 0.

## Working-directory / path convention (assumption A10)

Every path (`./polyphemus_files`, `./objects`, a voyage's `objects/xxx.png`, a stock
path) is stored and used exactly as given in its configuration/data file; nothing is
rewritten or prefixed. Phase 1 never actually opens the voyage image files, so the
`objects/` fixture directory under `nostos/` only matters for a *later* phase that does.
Run the executables with `nostos/` as the working directory (as the commands above do)
so relative paths resolve the way they would on the grading machine.

## Fixture configuration files (assumption A12)

No `config.dat` files were supplied with the course materials — only the binary stock
files, `voyages.dat`, and the PNG assets were. `configs/*.dat` were authored from the
statement's documented formats (P PDF pp. 11-14) and are clearly self-authored fixtures,
not supplied grading data. Sample IPs (`172.16.214.x`) match the statement's own
illustrative example. Ports reuse the assigned range **8620-8624** (7 endpoints are
needed across Ithaca + 6 islands; only 5 ports were assigned, so ports are reused across
distinct sample IPs) — configuration values only, nothing is ever actually listened on
in Phase 1.

Each island's `--- ROUTES ---` section lists the other five islands as *candidate*
routes with distinct IP/port pairs; the real `SPHRAGIS_filter_island_configuration()`
decides which survive. No topology is hardcoded or guessed — see
`docs/audit-handoff.md` for the observed (real, library-decided) survivor counts per
island, all of which cross-check as bidirectional.

## Commands (Phase 1: syntax-only)

Commands are intentionally **not** implemented yet — only recognized, syntax-validated,
and their arguments extracted, per the statement (P pp. 15-16, 19) and the official test
sheet. `ACCEPT 999`, `SAIL Atlantis`, `BUY UnknownProduct 3` etc. all print
`Command OK` even with no real connection/state, by explicit project rule. See
`include/commands.h` for the exact usage strings and `docs/assumptions.md` (A05-A09,
A17) for the documented lexical/grammar choices where the statement leaves room for
interpretation.

Type `CTRL+D` (EOF) or `CTRL+C` at any point to exit Odysseus cleanly.

## Tests

```sh
make test        # tests/run_functional_tests.sh — 60 reproducible checks (currently
                  # all passing; see docs/test-results.md for the actual run log)
make memcheck     # tests/run_memcheck.sh — Valgrind over the CTRL+C and representative
                  # failure paths of all three executables
```

`tests/loader_check.c` is a small **test-only** diagnostic (not part of `make all`) that
links the real loaders and dumps every loaded field, used by `run_functional_tests.sh`
to prove field-level storage (not just startup messages). It intentionally uses
`printf`/`fprintf` because it is a verification harness, never delivered as part of the
graded `odysseus`/`ithaca`/`island` binaries — see its file header and
`docs/test-results.md` for why it is exempt from the forbidden-API audit. Build it with:

```sh
gcc -D_GNU_SOURCE -Iinclude -Ilib -std=gnu11 -Wall -Wextra -g \
    -o tests/loader_check tests/loader_check.c \
    src/config.c src/text.c src/io.c src/voyages.c src/stock.c src/routes.c lib/sphragis.o
```

## Documentation

- `docs/testing-walkthrough.md` — step-by-step guide for testing this build yourself
  (build, run each executable, run the automated suite, run Valgrind, try failure cases).
- `docs/design.md` — module/ownership design (prepared for instructor validation; see
  `docs/assumptions.md` A02 for the design-validation status, which is **not yet
  obtained**).
- `docs/assumptions.md` — every open question (A01-A21) and the default this
  implementation takes, clearly marked as a documented choice, not an instructor ruling.
- `docs/report.md` — the evolving progress report (cover, design rationale, problems
  encountered, pending fields).
- `docs/test-results.md` — exact commands run, platform/compiler versions, actual
  captured results, and what remains unrun (Montserrat verification).
- `docs/audit-handoff.md` — requirement-to-file/function mapping and evidence index for
  an independent reviewer.

## What is intentionally not here

No sockets, `bind`/`listen`/`accept`/`connect`, forks, threads, IPC objects, market
transactions, real map rendering, food timers, mission-state mutation, file transfers,
or glyph signing/checking — all excluded by the Phase 1 scope. See
`PHASE1_IMPLEMENTATION_GUIDE.md` section 2.2 for the complete exclusion list.
