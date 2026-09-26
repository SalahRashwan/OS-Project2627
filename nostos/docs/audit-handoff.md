# Audit handoff — Nostos Phase 1 (re-audit after corrections)

Prepared for the independent reviewer. The first audit (`../../PHASE1_AUDIT_RESULTS.md`,
26 September 2026) reviewed an earlier version of this directory. This document maps
each of its findings to the correction made and the evidence produced, then gives the
requirement mapping and the exact commands for re-checking. Results are recorded in
`docs/test-results.md`; interpretation choices are in `docs/assumptions.md`.

Everything marked "passed" below was run on WSL2 Ubuntu 24.04 (GCC 13.3.0,
Valgrind 3.22.0). **Montserrat has not been used.** Nothing here claims instructor
approval.

## 1. Response to the audit findings

| Finding | Correction | Where | Regression evidence |
| --- | --- | --- | --- |
| **F1** (P1) double free of stored route strings after a later parse failure | Each route line is parsed into strings local to one call, initialized to `NULL` per line; every error branch frees only those. Ownership moves into the list only on a successful append. `loadIslandConfig` now closes/destroys through one cleanup path. | `src/config.c`: `appendRawRouteLine`, `loadIslandRawRoutes`, `parseRouteLine`, `loadIslandConfig` | `make faults`: every allocation call of Aeaea (46) and Scheria (47) failed in turn under Valgrind; malformed later route (`island_bad_second_route`) in `make test`, `make faults`, and `make memcheck`. The same fault suite run against the audited code reproduces the double free at allocation indices 18, 22, 26, 30 (as in the audit), so the test does detect F1. |
| **F2** allocation failures reported as `Unknown command` | New `PARSE_ERROR` result, returned by `parseCommand`, `classifySail`, `classifyBuySell` on any allocation failure. `PARSE_UNKNOWN` is only an unrecognized verb. The terminal prints `Error: out of memory while parsing a command.` on stderr, cleans up, exits 2. | `include/types.h` (`eParseStatus`), `include/commands.h` (API doc), `src/commands.c`, `src/Odysseus.c: executeAndPrint` | `make faults`: `ody_map_nl`, `ody_sail`, `ody_buy_status_eof` sweeps fail every allocation; the suite rejects any `Unknown command` for valid input and any exit 2 without stderr. |
| **F3** EOF remainder allocation failure silently dropped the last command | `handleEofRemainder` returns a status; `LINE_NONE` (nothing left) vs. `NOSTOS_ERROR` (allocation failure) are distinguished; failure is reported and propagated to exit 2. | `src/Odysseus.c: handleEofRemainder, runOdysseusTerminal` | `make faults`: `ody_map_eof`, `ody_buy_status_eof` (input without trailing newline); `make test`: `eof_without_newline`, `eof_partial_usage`. |
| **F4** terminal output failures discarded | Every prompt, result line, and usage line is checked (`printPrompt`, `printParseResult`, `executeAndPrint`, `processAvailableLines`); read/poll/signal/line-buffer failures also report. Diagnostics are fixed literals written once to stderr (no allocation, no retry). Ithaca/Island check their readiness *and* shutdown messages and report on stderr. | `src/Odysseus.c` (`reportTerminalError`, `ERROR_*` in `include/Odysseus.h`); `src/Ithaca.c: runIthacaLifecycle`; `src/Island.c: runIslandLifecycle`; `include/Ithaca.h`, `include/Island.h` | `make faults`: stdout writes failed from each index onward for Odysseus (1..6), Ithaca (1..3, including the shutdown message after SIGINT), Island (1..5); each must exit 2 with a stderr diagnostic and a clean Valgrind log. |
| **F5** style deviations | (a) All system includes moved into each module's own header; `.c` files include only project headers. (b) Every function definition now carries its own `@Name/@Def/@Arg/@Ret` header with an "argument = meaning" entry per parameter; no `See x.h` stubs remain. (c) `bIsVoyages/bIsMarket/bEof/bHasNumber` → `nIsVoyages/nIsMarket/nEof/nHasNumber` (`pbEof` → `pnEof`). (d) Constant-first ordering applied wherever one operand is a constant (`1 > lValue`, `INT_MAX < lAmount`, `0 > nResult`, `STOCK_RECORD_SIZE > nTotal`, ...); variable-vs-variable comparisons left as they are. Temporary realloc pointers renamed by type (`pstTemp`, `psTemp`, `appsTemp`). | all of `src/`, `include/` | `tests/style_check.py` (run first by `make test`): 0 findings on `src/`, `include/`, `tests/*.c`. The same checker reports 200+ findings on the audited copy (header stubs, includes, `b` prefixes, 11 constant-on-right comparisons, 28 stdio calls in the old `loader_check.c`), so it is not vacuous. It cannot judge naming quality or comment usefulness; that remains human review. |
| **F6** Odysseus "SIGINT" memcheck actually tested EOF; weak checks | `run_memcheck.sh` rewritten: Odysseus stdin is a FIFO held open by the script, the case waits for readiness, sends `MAP\nSTAT` (partial line), requires the process still alive, sends SIGINT, waits with a bound, asserts exit 0 and exactly one `Command OK`. Every case asserts its exit status; logs must show 0 errors, 0 bytes in use, and no open descriptor beyond 0-2 that is not inherited (parsed from the `--track-fds` entries). Missing Valgrind → `NOT RUN`, exit 77, never success. | `tests/run_memcheck.sh` | 11/11 clean (4 SIGINT, 1 EOF, 6 failure paths). The descriptor parser was checked against a synthetic log with a leaked descriptor (reported as a failure). Separately, `tests/pty_ctrl_c.py` sends a *terminal-generated* CTRL+C (0x03 on a pseudo-terminal) to all three programs: exit 0. |
| **F7** `make test` used a stale/missing checker; count-only assertions | `tests/bin/loader_check` is a real Makefile target built from `tests/loader_check.c` and the current loader objects; `make test` depends on it; `make clean` removes `tests/bin`. A missing checker or binary is a FAILURE. Assertions now `diff` every loaded field against values decoded independently (awk for text configs/voyages, `od` for the binary stock), check each filtered route keeps its raw IP/port, and check exit statuses and stderr on every case. `loader_check.c` itself rewritten with descriptor I/O and full style, so no forbidden-API exemption is needed. | `Makefile`, `tests/run_functional_tests.sh`, `tests/loader_check.c` | `make test`: 100/100. With the checker removed the suite reports 29 failures (not a skip). |

Delivery items from the audit's section 4:

1. **Montserrat** — still not verified (no access from this environment). Pending.
2. **Real `G<group>_F1.tar`** — `make package GROUP=<n>` now actually builds the tar
   (refuses to run without a group number). A trial archive was built, listed,
   extracted into an empty directory, built, and run (see `docs/test-results.md` §8).
   The real archive needs the real group number.
3. **Report fields** — group number and hours remain pending; not invented.
4. **Instructor design validation** — not obtained; unchanged (assumption A02).
5. **Interpretation choices** — unchanged and still labeled as documented defaults
   (A05-A09, A17, A03).
6. **Test-helper exemption** — removed: `tests/loader_check.c` no longer uses stdio.
   `tests/fault_inject.c` uses only `snprintf` (in-memory) and `write`, and exists only
   in test builds.
7. **Documentation claims** — `README.md`, `docs/test-results.md`, `docs/report.md` §5,
   `docs/design.md` §2/§5, `docs/testing-walkthrough.md`, and this file were updated to
   what was actually run.

## 2. How to re-audit

From `nostos/` on a GNU/Linux host with gcc, make, python3, and valgrind:

```sh
make clean && make all      # three executables, expect no gcc warnings
make test                   # style_check (0 findings) + 100 functional checks
make memcheck               # 11 Valgrind cases
make faults                 # every allocation/stdout-write failure, under Valgrind
python3 tests/pty_ctrl_c.py # terminal-generated CTRL+C for all three programs
```

`make faults` takes several minutes because it runs about 350 Valgrind executions;
`FAULT_VALGRIND=0 make faults` runs the same invariants without Valgrind in seconds.
Allocation indices are discovered by a calibration run (`NOSTOS_FAULT_REPORT`), so the
suite adapts when code changes instead of relying on fixed numbers.

## 3. Implemented phase and exclusions

Phase 1 only. No sockets, forks, threads, IPC objects, trading, real map, food timers,
mission-state changes, file transfers, glyph signing, or persistence. Signal handling
uses blocked SIGINT + `signalfd` + `poll` with no signal handler at all.

## 4. Requirement mapping (F01-F20)

| ID | Requirement | Files / functions | Evidence |
| --- | --- | --- | --- |
| F01 | Three executables, exact CLI forms | `Odysseus.c`, `Ithaca.c`, `Island.c` `main()`; `Makefile` | clean build; `*_no_args` exit-1 tests |
| F02 | All Odysseus config fields stored | `config.c: loadOdysseusConfig`, `loadOdysseusIdentity`, `loadOdysseusResources`, `loadOdysseusFoods` | `make test`: `odysseus_fields` (diff vs. awk decode) |
| F03 | Ithaca config + voyages, internal IDs | `config.c: loadIthacaConfig`; `voyages.c: loadVoyages`, `parseVoyageLine` | `ithaca_fields` (all 24 records, IDs 1..24, every field) |
| F04 | Island config + raw endpoints | `config.c: loadIslandConfig`, `loadIslandHeader`, `loadIslandRawRoutes`, `appendRawRouteLine` | `*_config_fields` for all six islands |
| F05 | Real Sphragis filtering, endpoints kept | `routes.c: filterIslandRoutes`, `collectSurvivors`; `Island.c: loadIslandRoutesFiltered` | `*_filtered_routes` (count = library result = observed 2/1/1/1/3/2; each survivor keeps its raw IP:port) |
| F06 | Binary stock until EOF, truncation rejected | `stock.c: loadStockList`, `readFullRecord`, `decodeStockRecord` | `*_stock_fields` (48 records vs. `od` decode); `island_truncated_stock` |
| F07 | Persistent in-process structures | `include/types.h` | `docs/design.md` §2 |
| F08 | Only Odysseus has a terminal | only `Odysseus.c` reads `STDIN_FILENO` | source; server SIGINT tests run with stdin `/dev/null` |
| F09-F11 | Eleven commands, case-insensitive, exact messages | `commands.c`, `include/commands.h` | `make test` command matrix (official sheet + edges) |
| F12 | No state checks | `commands.c` has no access to any list | `accept_absent_id`, `sail_unknown_island`, `buy_unknown_product` |
| F13 | Ithaca/Island block until CTRL+C | `lifecycle.c: waitForSignalOnly` | SIGINT tests; pty CTRL+C; 0 CPU ticks idle over 3 s (all three) |
| F14 | Release everything on CTRL+C/EOF/error | `run*Lifecycle`, `initialize*`, loader cleanup paths | `make memcheck` 11/11; `make faults` |
| F15 | Required I/O, forbidden APIs | `io.c` (only `read/write/open/close`, `vasprintf`) | `style_check.py` prohibited-call scan: 0 |
| F16 | No busy waits, no warnings | `poll(..., -1)` only | idle CPU check; gcc output |
| F17 | Makefile + reusable modules | `Makefile`, 11 `src/*.c`, 13 `include/*.h` | build |
| F18 | Self-contained tar, fresh build | `make package GROUP=<n>` | trial archive extracted/built/run (§8 of test results); Montserrat pending |
| F19 | Course conventions | all authored code | `style_check.py` + human review |
| F20 | Design validation honestly recorded | `docs/assumptions.md` A02 | **not obtained** |

## 5. Ownership summary

| Owning type | Loader | Destructor |
| --- | --- | --- |
| `tOdysseusConfig` | `loadOdysseusConfig` | `destroyOdysseusConfig` |
| `tIthacaConfig` | `loadIthacaConfig` | `destroyIthacaConfig` |
| `tVoyageList` | `loadVoyages` | `destroyVoyageList` |
| `tIslandConfig` | `loadIslandConfig` + `filterIslandRoutes` | `destroyIslandConfig` |
| `tRouteList` (raw or valid) | `loadIslandConfig` / `filterIslandRoutes` | `destroyRouteList` |
| `tStockList` | `loadStockList` | `destroyStockList` |
| `tParsedCommand` | `parseCommand` | `destroyParsedCommand` (safe after every result, including `PARSE_ERROR`) |

Every loader resets its output first and releases it on its own failure; list counts
include only fully owned entries; every `realloc` goes through a temporary pointer.

## 6. Known limits and unverified points

- **Montserrat not tested.** Run the commands in §2 there before submission.
- **Sphragis error path (A19).** On a negative library return the adapter frees any
  non-NULL slot of its temporary array. The six fixture islands never produce a
  negative return, so this path is still unexercised against the real library.
- **Fault injection scope.** The wrappers count allocation calls made by linked
  objects (project code and `sphragis.o`); allocations made inside libc itself (for
  example inside `strtok_r`, which makes none, or `open`) are not injected.
  `vasprintf` is wrapped as a whole.
- **Group number, hours, design validation** — pending, owned by the student.

## 7. Reviewer checklist

- [x] Three programs build with no gcc warnings (WSL2).
- [x] Every loaded field compared with an independent decode.
- [x] Real Sphragis filtering with endpoint preservation.
- [x] Every command form, case handling, numeric validation, exact messages.
- [x] No premature state checks, no Phase 2/3 features.
- [x] CTRL+C (signal and terminal-generated) and EOF shutdown, no busy wait.
- [x] Every allocation and stdout-write failure handled without crash, leak, invalid
      free, lost command, or silent failure (fault suite, Valgrind).
- [x] Mechanical style rules: 0 findings; human style review still welcome.
- [ ] Montserrat build and test run.
- [ ] Real `G<group>_F1.tar` (needs the group number).
- [ ] Instructor design validation (not obtained).
