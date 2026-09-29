# The Nostos System, Phase 1: Inithaca

Phase 1 of the Nostos System (OS 2026-27): startup of the Odysseus, Ithaca and Island
processes, loading of their configuration and data files, route filtering with the
Sphragis library, and a command parser for Odysseus that checks syntax only. Sockets,
concurrency, trading and persistence are not part of this phase.

The progress report is in `report/report.md`.

## Prerequisites

- GNU/Linux (Montserrat is the target machine; developed and tested on Ubuntu 24.04
  under WSL2).
- `gcc` with `-std=gnu11` (tested with GCC 13.3.0).
- `valgrind` and `python3` for the test targets.

## Build

```sh
make                     # builds odysseus, ithaca, island
make clean               # removes objects, executables, test binaries and logs
make package GROUP=<n>   # creates G<n>_F1.tar
```

Only `island` is linked with `lib/sphragis.o`. The build uses `-Wall -Wextra` and
produces no warnings.

## Run

```sh
./odysseus <config.dat>
./ithaca <config.dat> <voyages.dat>
./island <config.dat> <stock.db>
```

With the included files:

```sh
./odysseus configs/odysseus.dat
./ithaca configs/ithaca.dat data/voyages.dat
./island configs/aeaea.dat data/stocks/Aeaea.db
```

Each island is started with its own config and stock file:
`configs/{aeaea,aeolia,ismarus,ogygia,scheria,thrinacia}.dat` with
`data/stocks/{Aeaea,Aeolia,Ismarus,Ogygia,Scheria,Thrinacia}.db`.

Only `odysseus` has a terminal. `ithaca` and `island` print their startup messages and
then wait, without using CPU, until **CTRL+C**; they then print a shutdown message, free
everything and exit with status 0. Odysseus ends on CTRL+C or CTRL+D.

Run the programs from the `nostos/` directory. Paths inside the configuration and data
files (for example `objects/xxx.png`) are used exactly as written, relative to the
current directory. Phase 1 stores the image paths but does not open the images.

## Configuration files

No `config.dat` files were provided with the statement, so the files in `configs/` were
written by us following the formats in the statement. The IPs (`172.16.214.x`) are
examples; the ports are from our assigned range 8620-8624 and are reused across
different IPs, since seven endpoints are needed. Nothing listens on them in Phase 1.

Each island's `--- ROUTES ---` section lists the other five islands as candidates, and
`SPHRAGIS_filter_island_configuration()` decides which routes are kept. The IP and port
of each kept route are taken from the configuration file.

## Commands

Phase 1 only recognizes the commands, checks their syntax and extracts their arguments.
A valid command prints `Command OK`, an unknown one prints `Unknown command`, and a
known command with wrong arguments prints its usage line (see `include/commands.h`).
State is not checked yet, so `ACCEPT 999` or `SAIL Atlantis` print `Command OK`.

Where the statement leaves room for interpretation we chose:

- Command words are case-insensitive; arguments keep their spelling.
- Numbers must be digits only: no sign, no decimals, leading zeros allowed. Voyage IDs
  and amounts must be between 1 and `INT_MAX`.
- A blank line prints nothing and shows the prompt again.
- `LIST` with a missing or unknown second word prints both `LIST` usages.
- `BUY MAP` only accepts amount 1; `SELL` has no special case for `MAP`.
- Voyage IDs are not in `voyages.dat`, so they are assigned 1, 2, 3... in file order.
- Exit status: 0 after CTRL+C/EOF, 1 for wrong arguments, 2 for any file, memory or
  I/O error (always with a message on stderr).

## Tests

```sh
make test      # style check + functional tests (commands, EOF, exit statuses,
               # CTRL+C shutdown, every loaded field compared with the input files)
make faults    # makes every allocation and every screen write fail, one at a time,
               # under Valgrind (FAULT_VALGRIND=0 runs it without Valgrind, faster)
make memcheck  # Valgrind on CTRL+C, EOF and initialization-error runs
make pipes     # stdout/stderr connected to pipes with no reader, run plainly
               # and under Valgrind
make check     # all of the above
```

Test-only files, never linked into the three programs:

- `tests/loader_check.c`: prints every field the loaders stored, so the tests can
  compare it with the input files.
- `tests/fault_inject.c`: wrappers used by the `tests/bin/*-fault` builds to make a
  chosen allocation or write fail.
- `tests/style_check.py`: checks the style-guide rules that can be checked
  automatically.
- `tests/pty_ctrl_c.py`: sends a real terminal CTRL+C to each program.
- `tests/pipe_tests.py`: runs each program with its stdout and/or stderr on a pipe
  whose reader is closed, and checks it exits with an error status instead of being
  killed by SIGPIPE.

## Error handling

Every allocation, read and write is checked. On an error the program prints a message
on stderr, frees everything it owns and exits with status 2.

Each program sets SIGPIPE to be ignored as the very first step of `main()`. Without
that, writing to a pipe whose reader is gone (for example `./ithaca ... | head -1`
after `head` exits) kills the process before it can clean up. With SIGPIPE ignored the
write returns `EPIPE`, which goes through the normal write-error path above.

## Not included in Phase 1

Sockets, forks, threads, IPC, trading, the real map, food timers, mission state, file
transfers and glyph signing are all left for later phases.
