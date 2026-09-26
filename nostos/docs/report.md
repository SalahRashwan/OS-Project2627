# The Nostos System — Progress Report

**Project:** The Nostos System (Operating Systems, La Salle)
**Academic year:** 2026-27
**Phase:** 1 — Inithaca
**Student:** Salah Ahmed Salaheldin Adly Rashwan
**Group:** *pending — not established by this handoff; fill in before submission
(assumption A15)*
**Date:** 26 September 2026 (evolving document; first version 21 September 2026)

This is a **progress report**, written for the Phase 1 partial delivery the statement
requires (P PDF p.39). It describes exactly what was implemented, why, and what remains
open — it does not claim final-submission status or instructor validation.

## 1. Table of contents

1. Table of contents
2. Design overview
3. Data structures and justification
4. Resources/APIs used and why
5. Problems encountered and actual solutions
6. Block diagram
7. Time spent
8. Conclusions and proposed improvements
9. Theme note
10. Bibliography

## 2. Design overview

Three independent, non-forking processes: `odysseus`, `ithaca`, `island`. Each is built
from one entry-point file (`Odysseus.c`/`Ithaca.c`/`Island.c`) plus reusable modules
(`io`, `text`, `config`, `commands`, `lifecycle`, `voyages`, `stock`, `routes`) so that
`island` alone represents any of the six islands purely by configuration, as required.
No sockets, forks, threads, or IPC objects exist anywhere in the codebase — Phase 1
explicitly forbids them, and the full module/ownership design (with its rationale) is
recorded separately in `docs/design.md` so it can be reviewed independently of this
narrative report.

## 3. Data structures and justification

Every aggregate (`tOdysseusConfig`, `tIthacaConfig`, `tVoyageList`, `tIslandConfig`,
`tRouteList`, `tStockList`) is a small typed struct plus a manually grown dynamic array
(count/capacity, `realloc` through a temporary pointer) — never a fixed-size buffer sized
to today's fixtures (24 voyages, 8 products, 5 candidate routes). This was a deliberate
choice: the guide and the statement both warn against hardcoding fixture-derived
constants, and a bigger `voyages.dat` or a stock file with more/fewer products must
change the program's behavior, not just its printed numbers. See `docs/design.md`
section 2 for the full ownership table (who allocates what, who frees it, and when).

## 4. Resources/APIs used and why

- `open`/`close`/`read`/`write` only for descriptor I/O, per the explicit project
  restriction; `asprintf`/`vasprintf` for in-memory formatting (permitted), never
  `printf`/`fprintf`/`fgets`/etc. in any of the three delivered executables' source.
- `sigprocmask` + `signalfd` + `poll`, not `signal()`/handler+flag: the course UNIX book
  explains the classic check-then-`pause()` race (12.4) that a naive
  `while (!flag) pause();` design falls into; blocking SIGINT for the entire process
  lifetime and only ever observing it through a `poll()`-readable descriptor avoids that
  race by construction, and means no signal-handler code exists anywhere in this
  codebase (so the "no allocation/formatting/freeing in a handler" rule cannot be
  violated even by accident).
- `strtok_r` (not `strtok`) for tokenization, since a nested/repeated call (e.g. parsing
  many food lines in a loop) must not corrupt another call's parsing state.
- `strtol` (not `atoi`) for every numeric field, since `atoi` cannot distinguish a
  genuinely invalid token from a legitimate `0`.

## 5. Problems encountered and actual solutions

- **Sphragis ownership.** The library frees rejected destination names in place and
  expects separately heap-allocated strings it is allowed to free. The raw route list
  already needs its own owned name/IP strings (to preserve IP/port for survivors, which
  the library itself does not track), so handing those directly to the library would let
  it free strings the raw list still thinks it owns. Solution: build a throwaway array of
  *copies* just for the library call, then match survivors back to the raw list by
  case-insensitive name and move (not re-copy) each survivor's string into a fresh valid
  list. Full algorithm in `docs/design.md` section 4; verified leak/double-free-free
  under Valgind (`docs/test-results.md` section 6).
- **Binary stock layout.** Reading `struct { char name[100]; int amount; int price; }`
  directly risks the compiler inserting padding that the on-disk layout does not have,
  and reading an `int` in place risks endianness/`sizeof(int)` assumptions. Solution:
  read exactly 108 raw bytes and decode the two integers byte-by-byte as little-endian,
  independent of the host's struct layout. Verified against an independent Python
  `struct.unpack` decode of the same files before any C code touched them.
- **EOF vs. a blank line vs. a partial final record.** A `read()` returning 0 mid-line
  (a truly empty final line) is different from `read()` returning 0 with nothing
  buffered (true EOF), which is different from `read()` returning 0 after a partial
  binary record (truncation — an error, never treated as a complete record). All three
  are distinguished explicitly in `text.c`/`stock.c`; see `docs/assumptions.md` and the
  truncated-stock-file test in `docs/test-results.md`.
- **Odysseus's simultaneous stdin/CTRL+C wait.** A CTRL+C that arrives after a partial
  command line has already been typed must still terminate cleanly and discard that
  partial line — a design that only checks stdin cannot see it. Solution: `poll()` on
  both stdin and the signalfd every iteration, so a signal-ready descriptor is checked
  before (and independent of) whatever partial line sits in the accumulator. Verified
  with a FIFO-driven test that sends a partial command, waits, then sends a real
  `SIGINT`.
- **Independent audit (26 September 2026) and its corrections.** An external audit
  (`PHASE1_AUDIT_RESULTS.md`) found that the ordinary tests only covered successful
  allocations and writes. Injecting failures exposed four runtime defects: a double
  free in the island route loader when a later route line failed after an earlier route
  had been stored (F1); parser allocation failures printed as `Unknown command` (F2); an
  allocation failure while extracting a final unterminated command silently dropped it
  (F3); and failed terminal writes were ignored (F4). It also found style-guide gaps (F5)
  and two weaknesses in our own tests: the Odysseus "SIGINT" memory test could really be
  an EOF run (F6), and `make test` could run a stale or missing field checker (F7).
  Solutions: each route line is now parsed into fresh, iteration-local strings that are
  freed only by that iteration (`config.c: appendRawRouteLine`); the parser has a
  separate `PARSE_ERROR` result; every terminal output and EOF step returns a status
  that is propagated, reported with a fixed stderr message, and turned into exit 2 after
  full cleanup; the style pass moved system includes into headers and completed every
  definition header. The lesson was that "0 bytes leaked" in a normal run proves little
  about error paths, so the tests now fail every allocation call and every stdout write
  one at a time under Valgrind (`tests/run_fault_tests.sh`), and the same suite run
  against the pre-correction code reproduces the audit's double free at the same
  allocation indices.

## 6. Block diagram

See `docs/design.md` section 1 for the module map (kept there rather than duplicated,
to avoid the two documents drifting apart). It is explicitly labeled as showing no live
network connections, since Phase 1 has none — any future network diagram belongs in a
Phase 2 document, not here.

## 7. Time spent

*Pending — actual historical work-hour records were not supplied to this handoff and
are not invented here (assumption A15). Fill in per student, per category (research,
design, implementation, testing, documentation) before submission.*

| Student | Research | Design | Implementation | Testing | Documentation |
| --- | ---: | ---: | ---: | ---: | ---: |
| Salah Ahmed Salaheldin Adly Rashwan | pending | pending | pending | pending | pending |

## 8. Conclusions and proposed improvements

Phase 1's real constraint is discipline, not difficulty: every loader must handle
partial reads, every allocation must be checked, and every failure path must release
exactly what it acquired — none of that is visible in a demo run that only exercises the
happy path. The most valuable single piece of test evidence gathered this session was
the idle-CPU check (0 ticks over 3 seconds while blocked), because it is the one result
that distinguishes a correct `poll()`-based wait from a wait that merely *looks* correct
in casual testing but secretly spins.

For Phase 2, the parser (`commands.c`), loaders, and Sphragis adapter should not need to
change shape — only new state (a "connected", "docked", "active voyage" model) needs to
be threaded through, and the same command-dispatch structure can grow real handlers
alongside the existing syntax validators without rewriting them.

## 9. Theme note

Optional per the statement: The Nostos System follows the Odyssey — Ithaca is home and
the mission dispatcher, Odysseus is the traveling client seeking to complete voyages and
return, and each Island is one of the ports of call from the epic (Aeaea being Circe's
island, Aeolia the island of Aeolus, etc.), each with its own trade goods and known
neighbors, exactly mirroring the "known but not fully mapped" geography of the original
poem.

## 10. Bibliography

- `Phase1/OS_Project_2026-27_ICE_cleaned.pdf` — the Nostos System statement, Phase 1
  sections (PDF pp. 11-20, 38-40).
- `Phase1/Style Guide.pdf` — mandatory student-code conventions.
- `Phase1/OS_Project_2025-26 - testing Phase 1.pdf` — official Phase 1 test cases.
- `Phase1/Sphragis libray/sphragis.h` — the real Sphragis library contract.
- `All Materials/Other materials/UNIX-Programming-2026-27-en.pdf` — descriptor I/O,
  `strtol`/`strtok_r`, `sigaction`/`signalfd`/`poll`, EINTR/short-read handling.
- `All Materials/Codeing samples/` — File Descriptors, Signals, and Select sample
  groups, used as the taught methodology (with their deliberate simplifications/races
  not carried over — see `PHASE1_IMPLEMENTATION_GUIDE.md` section 1.4 for the explicit
  per-file lesson/limitation mapping).
