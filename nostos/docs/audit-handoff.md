# Audit handoff — Nostos Phase 1

Prepared for an independent reviewer (Codex). This document maps every guide
requirement to real files/functions and to the actual evidence gathered in this
session. It distinguishes what was verified from what remains pending; see
`docs/test-results.md` for full logs/transcripts and `docs/assumptions.md` for every
documented interpretation choice.

## 1. Implemented phase and explicit exclusions

Phase 1 only. No sockets, `bind`/`listen`/`accept`/`connect`, forks, threads, IPC
objects (queues/shared memory/semaphores), trading, real map rendering, food timers,
mission-state mutation, file transfers, glyph signing/checking, or persistence exist
anywhere in `src/`/`include/`. Confirmed by source inspection (grep for
`socket|fork|pthread|shmget|msgget|semget` across `src/*.c include/*.h` returns no
matches) and by the fact that only `<unistd.h> <fcntl.h> <signal.h> <poll.h>
<sys/signalfd.h> <stdarg.h> <stdlib.h> <string.h> <ctype.h> <errno.h> <limits.h>
<stdint.h> <sys/types.h> <stddef.h>` plus the project's own headers are included
anywhere.

## 2. Requirement mapping (F01-F20)

| ID | Requirement | Files / functions | Evidence |
| --- | --- | --- | --- |
| F01 | Three executables, exact CLI forms | `src/Odysseus.c`, `src/Ithaca.c`, `src/Island.c` `main()`; `Makefile` | `docs/test-results.md` §2 (clean build), §8 (fresh-extraction run) |
| F02 | All Odysseus config fields stored | `config.c: loadOdysseusConfig/loadOdysseusIdentity/loadOdysseusResources/loadOdysseusFoods` | `tests/loader_check odysseus`; `docs/test-results.md` §5 |
| F03 | Ithaca config + voyages, internal IDs | `config.c: loadIthacaConfig`; `voyages.c: loadVoyages/parseVoyageLine` (ID = `pstList->nCount + 1`, assumption A04) | `tests/loader_check ithaca`: 24 voyages, IDs 1..24, `docs/test-results.md` §5 |
| F04 | Island config + raw route endpoints | `config.c: loadIslandConfig/loadIslandHeader/loadIslandRawRoutes` | `tests/loader_check island`: `island.rawRoute[*]` fields |
| F05 | Real mandatory Sphragis filtering | `routes.c: filterIslandRoutes` (calls the real, linked `SPHRAGIS_filter_island_configuration`); `Island.c: loadIslandRoutesFiltered` calls it unconditionally before any route is stored valid | `docs/test-results.md` §4 (six real, library-decided, bidirectional-consistent results) |
| F06 | Binary stock records until EOF | `stock.c: loadStockList/readFullRecord/decodeStockRecord` | All six real `.db` files decode to 8 products each; truncation test (`docs/test-results.md` §3, §6) |
| F07 | Persistent in-process structures | `include/types.h` (all `tXConfig`/`tXList` types); one instance per process, whole-process lifetime | `docs/design.md` §2 |
| F08 | Only Odysseus has a terminal | `Ithaca.c`/`Island.c` never read `STDIN_FILENO`; only `Odysseus.c` does (`handleStdinReadable`) | Source inspection; three concurrent sessions run in this work (`docs/test-results.md` §3) |
| F09 | All eleven command forms recognized | `commands.c: dispatchVerb` + eleven `classify*` functions | `tests/run_functional_tests.sh` — 60/60 passing, covers all eleven |
| F10 | Case-insensitive + numeric validation | `strcasecmp` throughout `commands.c`; `text.c: parseDigitsToLong` | Case-insensitivity + numeric-junk/overflow/leading-zero tests, all passing |
| F11 | Exact success/unknown/usage messages | `include/commands.h` (`COMMAND_OK_MESSAGE`, `UNKNOWN_COMMAND_MESSAGE`, `USAGE_*`) | Official T p.2 matrix passes byte-for-byte (`docs/test-results.md` §3) |
| F12 | No semantic/state checks | `commands.c` never touches a voyage/route/stock list; no such parameter exists in its signature | `ACCEPT 999`/`SAIL Atlantis`/`BUY UnknownProduct 3` all `Command OK`, tested |
| F13 | Ithaca/Island init → alive → CTRL+C exit | `lifecycle.c: waitForSignalOnly`; `Ithaca.c/Island.c: run*Lifecycle` | 0-CPU-tick idle check + SIGINT shutdown, `docs/test-results.md` §3 |
| F14 | All processes free resources on CTRL+C/error | Every `run*Lifecycle`/`initialize*` releases exactly what it acquired, in every branch | Valgrind: 0 errors, 0 bytes leaked, all six cases (`docs/test-results.md` §6) |
| F15 | Required I/O / forbidden-API restrictions | `io.c` (only `read`/`write`/`open`/`close`, `vasprintf`) | §5 below: forbidden-API scan, zero hits |
| F16 | No busy waits/crashes/warnings | `lifecycle.c` (`poll()` infinite timeout, never zero-timeout); `Makefile` (`-Wall -Wextra`, zero warnings) | `docs/test-results.md` §2 (build), §3 (idle CPU) |
| F17 | Makefile + reusable modules | `Makefile`; `src/*.c` (11 files), `include/*.h` (14 files), no monolithic single-file implementation | `docs/test-results.md` §2 |
| F18 | Correct tar, self-contained, fresh build | — | `docs/test-results.md` §8 (actually performed; Montserrat itself still pending) |
| F19 | Course coding conventions | All of `src/`/`include/` | §5 below (style audit) |
| F20 | Design validation status honestly recorded | `docs/assumptions.md` A02 | **Not obtained** — explicitly flagged, never claimed |

## 3. Module map and ownership

See `docs/design.md` §1 (module map) and §2 (the full per-type ownership table). Summary
of destructor pairing (every loader has a matching destroyer, safe on partial state):

| Owning type | Loader | Destructor |
| --- | --- | --- |
| `tOdysseusConfig` | `loadOdysseusConfig` | `destroyOdysseusConfig` |
| `tIthacaConfig` | `loadIthacaConfig` | `destroyIthacaConfig` |
| `tVoyageList` | `loadVoyages` | `destroyVoyageList` |
| `tIslandConfig` | `loadIslandConfig` + `filterIslandRoutes` | `destroyIslandConfig` |
| `tRouteList` (raw or valid) | `loadIslandConfig` (raw) / `filterIslandRoutes` (valid) | `destroyRouteList` |
| `tStockList` | `loadStockList` | `destroyStockList` |
| `tParsedCommand` | `parseCommand` | `destroyParsedCommand` |

## 4. Exact compiler/linker commands, platform, hashes

See `docs/test-results.md` §1-2 for the full transcript. Summary:

```
CC = gcc, CFLAGS = -std=gnu11 -Wall -Wextra -g -MMD -MP, CPPFLAGS = -D_GNU_SOURCE -Iinclude -Ilib
gcc 13.3.0, WSL2 Ubuntu 24.04.2 LTS, kernel 6.18.33.2-microsoft-standard-WSL2
lib/sphragis.o MD5 identical to Phase1/Sphragis libray/sphragis.o: 1be14ef0b94731413d8d46debf8f40c2
```

## 5. Style / forbidden-API audit (scoped to authored source only)

Scan of `src/*.c include/*.h` (never `All Materials`, the PDFs, or `lib/sphragis.o`):

```sh
$ grep -nE '\b(printf|fprintf|scanf|fscanf|gets|puts|getchar|fgets|fopen|fread|fwrite|getline|perror|system|popen|stat|fstat|lstat)\s*\(' src/*.c include/*.h
(no matches)
$ grep -n '\bgoto\b' src/*.c include/*.h
(no matches)
$ grep -n '?' src/*.c include/*.h   # ternary operator scan, excluding comment banners
(no matches)
```

`asprintf`/`vasprintf` (permitted in-memory formatting) are used exactly once, in
`io.c: writeFormatted`. Function-length audit (brace-depth script, cross-checked
manually per the guide's caveat that a naive regex is not fully reliable): every
function in `src/` is at or under 45 physical lines; the one that initially exceeded it
(`loadOdysseusHeader`, 53 lines) was split into `loadOdysseusIdentity` +
`loadOdysseusResources`, re-measured, and re-verified by rebuilding and re-running the
full test suite (still 60/60 passing).

`tests/loader_check.c` is the one exception: it is a **test-only diagnostic**, not part
of `make all` and not one of the three delivered executables, and uses `printf`/`fprintf`
deliberately (documented in its own file header and in `README.md`). It is excluded from
this scan's scope by design, the same way the guide excludes `All Materials` and the
vendor object.

Hungarian naming, function headers (`@Name`/`@Def`/`@Arg`/`@Ret`), file headers
(`@File`/`@Purpose`/`@Author`/`@Date`), four-space indentation, same-line braces,
constant-first comparisons, and no chained assignments were applied throughout by
construction; spot-check any file in `src/`/`include/` to verify.

## 6. Sphragis integration evidence

See `docs/test-results.md` §4 for the full six-island table (real, library-decided
route counts, cross-checked as bidirectional and endpoint-consistent) and §6 for the
Valgrind result on the `island` binary specifically (0 errors, 0 leaked bytes, 48
allocs/48 frees on the CTRL+C path that includes one real filtering call). The adapter
algorithm itself is in `routes.c: filterIslandRoutes`, documented step-by-step in
`docs/design.md` §4.

## 7. Archive listing and fresh-extraction evidence

Actually performed this session (not merely planned) — see `docs/test-results.md` §8 for
the full transcript: a real `tar` archive of this directory (88 entries, `build/` and
`tests/valgrind-logs/` excluded) was extracted into a clean `/tmp` directory with zero
access back to the original project files, built with `make clean && make all` (zero
warnings), and both `ithaca` (SIGINT after 0.5s, clean shutdown) and `odysseus` (two
real commands via stdin) were run successfully from that fresh copy. The actual
`G<group>_F1.tar` with the real group number still needs to be produced before
submission (group number pending, assumption A15).

## 8. Assumptions status (A01-A21)

See `docs/assumptions.md` for the full register with rationale. Status summary:

- **Resolved by explicit rule/source precedence:** A01, A11.
- **Implemented defaults (documented, not instructor-confirmed):** A04-A10, A12-A14,
  A16-A21.
- **Unresolved, needs the student/instructor:** A02 (design validation — **not
  obtained**), A03 (singleton Ithaca enforcement expectation), A15 (group number,
  teammates, actual hours).

## 9. Known issues, unsupported claims removed, unrun checks

- No known correctness defect as of this session's testing (60/60 functional tests, 6/6
  clean Valgrind cases, zero compiler warnings).
- **Not run:** Montserrat itself (§7 above is a real GNU/Linux stand-in, not proof of
  Montserrat acceptance); a literal keyboard-generated `SIGINT` in an interactive TTY
  session (only `kill -INT`, which delivers the identical signal Ithaca/Island/Odysseus
  observe, was used); allocation-failure fault injection (only real, naturally-occurring
  error paths — missing/malformed/truncated files — were exercised, not a wrapped
  `malloc` that fails on a chosen call).
- **Adapter defensive assumption (A19), not a proven library guarantee:** on a negative
  `SPHRAGIS_filter_island_configuration()` return, `routes.c` defensively frees any
  non-NULL slot left in the temporary name array, since the header does not document
  array mutation on the error path. This was never actually exercised against a real
  invalid-island/invalid-connection input in this session (only the success path was
  tested, since the six real fixture islands are all valid names) — a reviewer with
  Sphragis source access could confirm or refute this defensive choice directly; absent
  that, treat it as a documented, reasoned assumption, not a verified fact.
- No mocked Sphragis anywhere; the delivered `island` binary always links and calls the
  real `lib/sphragis.o`.

## 10. Student walkthrough for the interview

Short answers a student should be able to expand on (full context in `docs/design.md`
and `docs/report.md` §5):

- **read/write vs. printf/fgets:** required by the project; `read()` never
  NUL-terminates and can return fewer bytes than requested (or split a "line" across
  calls), so every loader must accumulate and check explicitly (`text.c:
  tLineBuffer`/`readNextLine`), unlike `fgets`, which hides that complexity (and is
  banned anyway).
- **EOF / blank line / error / partial record:** `read() == 0` with nothing buffered is
  EOF; a `\n` with nothing before it is a legitimate empty line; `read() < 0` is an
  error (distinguished from `EINTR`, which is retried); a binary record that stops
  partway through its 108 bytes is a truncation error, never silently counted as a
  complete product (`stock.c: readFullRecord`).
- **Ownership / partial-init cleanup:** every loader zeroes its output first, so its own
  destructor is always safe to call on it, success or failure; a later stage's failure
  only needs to clean up what earlier stages acquired, because each loader is
  self-cleaning on its own failure (see `docs/design.md` §7 for the exact acquisition
  order per process).
- **realloc via a temporary pointer:** a failed `realloc` returns `NULL` but leaves the
  original block valid; assigning the result directly to the only pointer would leak
  that block and lose the data. Every growable array here (`voyages.c`, `stock.c`,
  `config.c`'s route list) reallocs into a temporary first.
- **What Sphragis frees, and how endpoints survive it:** it frees rejected names inside
  the array it is given and compacts survivors in place; it never sees IP/port at all.
  The adapter hands it disposable name *copies*, then matches survivor names back to the
  original raw routes (which still own their IP/port) to rebuild a fresh valid list —
  see `routes.c: filterIslandRoutes`.
- **Why `ACCEPT 999` succeeds now:** Phase 1 is explicitly syntax-only (P p.15-16, 19);
  no voyage list, connection, or docking state is consulted by the parser at all.
- **Why stock records are fixed-size binary, and why ABI matters:** the statement
  specifies `char name[100]; int amount; int price;` on disk; reading it as a native
  struct risks compiler-inserted padding or endianness mismatches across machines, so
  this implementation decodes each field from raw bytes explicitly instead.
- **One CTRL+C, no unsafe handler cleanup:** SIGINT is blocked for the whole process and
  never has a handler; it is only ever observed by `poll()` reporting its `signalfd`
  readable, which happens exactly once and cannot race with the moment a wait begins
  (`docs/design.md` §6).
- **Idle vs. busy:** `poll(..., -1)` with an infinite timeout genuinely blocks in the
  kernel (0 CPU ticks measured over 3 idle seconds); a "busy" design would poll with a
  zero/short timeout in a loop and burn CPU continuously.
- **What Phase 2 would add:** real state (connected/docked/active-voyage) threaded
  through the same parser and loaders, sockets replacing the "load-and-store" endpoints
  that already exist, and stock mutation on top of the stock list already loaded — none
  of which requires reshaping `commands.c`'s dispatch structure or any loader's contract.
- **What was actually proven vs. what remains unverified:** see section 9 above,
  verbatim — do not overstate Montserrat readiness or the Sphragis error-path assumption
  beyond what is written there.

## Reviewer checklist (guide §18)

- [x] All three programs exist, build, and use the exact CLI forms.
- [x] No loader silently drops required data (field-level dump via `tests/loader_check`).
- [x] No hardcoded fixture counts/results (route/voyage/stock counts are all
      library/file-driven; verified by re-running against genuinely different-sized
      inputs — empty voyages/stock files, a longer/shorter route candidate list).
- [x] Correct real binary-stock layout and EOF/truncation handling.
- [x] Actual Sphragis filtering, correct success interpretation (nonnegative count, not
      `SPHRAGIS_OK`), ownership and endpoint matching (verified bidirectionally).
- [x] Every command recognized, case handled, arguments extracted, numeric conversion
      checked (60/60 functional tests).
- [x] Success/unknown/usage messages correct; extra tokens rejected.
- [x] No premature semantic checks or Phase 2/3 implementation.
- [x] Safe CTRL+C and EOF lifecycle without busy waits or signal-handler heap work
      (0 CPU ticks idle; no handler exists in the codebase at all).
- [x] Error exits release partial state and owned descriptors (Valgrind: 0 leaks across
      6 cases including 3 failure paths).
- [x] No warnings, prohibited API shortcuts, or accidental vendor deletion in clean
      (MD5-verified `lib/sphragis.o` unchanged by `make clean`).
- [x] Style guide applied, including Hungarian names, headers, includes, comments, and
      short functions (one violation found and fixed during this session; see §5).
- [x] Report, README, bibliography, and interview explanations match actual code.
- [ ] Genuine self-contained tar verified on **Montserrat** — a real fresh-extraction
      build/run was verified on WSL2 Ubuntu (§7), but Montserrat itself is pending.
- [x] Human design validation and ambiguous requirements honestly recorded (design
      validation explicitly **not** obtained; recorded, never fabricated).
