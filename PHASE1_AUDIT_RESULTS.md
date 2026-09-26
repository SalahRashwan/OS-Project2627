# Phase 1 audit results

Audit date: 26 September 2026. Reviewed submission: `Phase1Code/nostos/`.

**Verdict: the core Phase 1 functionality works in the tested Linux environment, but this version is not ready for submission without corrections.** I confirmed one high-priority memory-safety defect, three other runtime error-handling defects, strict style deviations, and two weaknesses in the supplied test infrastructure. The passing ordinary tests do not cover these failure paths.

This was an audit, not an implementation pass. The submitted source and its original logs were not edited. Independent builds, fault-injection helpers, test fixtures, and new logs are under `audit-work/`. That directory is audit evidence, not a submission directory.

## 1. Basis and scope

The project statement and style guide are the requirements; the previous implementation guide is supporting advice. Claims in Claude's README, design, test-results, and audit-handoff documents were treated as claims to verify, not instructions to follow or proof of correctness.

Authoritative local references, with **PDF page numbers**:

- `Phase1/OS_Project_2026-27_ICE_cleaned.pdf`: Phase 1 pp. 11–20; error handling, resource release, and prohibited operations p. 19; delivery requirements pp. 38–40.
- `Phase1/Style Guide.pdf`: system includes p. 5; variable prefixes pp. 10–11; function definition headers and 45-line limit p. 13; constant-first comparisons p. 15; comments pp. 18–19.
- `Phase1/OS_Project_2025-26 - testing Phase 1.pdf`: supplied official command cases. Its older filename was not treated as overriding the current statement.
- `Phase1/Rubric-2025-26.pdf`: execution, documentation, and interview criteria. This audit cannot assess the student's interview performance or assign an official grade.
- The original Sphragis header/object and the previously reviewed course examples and UNIX material. Course examples establish the descriptor-I/O and memory-management methodology; their simplified error handling does not override the project’s explicit robustness requirements.

Reviewed all 11 runtime C files, all 13 project headers, the Makefile, both test scripts, the loader-check source, configurations, supplied binary data, and implementation documentation. I rebuilt the runtime executables and the field checker independently. I did not audit later-phase networking or trading behavior because Phase 1 expressly excludes it.

## 2. Confirmed findings

Severity: **P1** = fix before submission because it can crash or corrupt memory. **P2** = required correction to error handling, compliance, or reliable verification. Severity is an audit priority, not a predicted mark deduction.

### F1 — P1: Island double-frees stored route strings after a later parsing failure

Location: [config.c:645](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/src/config.c:645>), particularly lines 645–648 and the successful append at 657–662. The second free occurs in `destroyRouteList`, lines 470–471.

`loadIslandRawRoutes` keeps `psDestination` and `psIp` across loop iterations. After `routeListAppend` succeeds, ownership moves into the route list, but these local pointers still point to those stored strings. If the next `parseRouteLine` fails before assigning new strings—for example, when its token-array allocation fails—the failure branch frees the previous route's strings. `failIsland` subsequently destroys the route list and frees the same allocations again.

**Independently reproduced:**

1. Retain the five header lines and first valid route from `configs/aeaea.dat`, then add a line containing `broken`.
2. Run `./island <that-config> data/stocks/Aeaea.db`.
3. The normal binary aborts with `free(): double free detected in tcache 2` (SIGABRT).
4. Valgrind reports two invalid frees, with the first frees at `config.c:647/648` and repeated frees at `config.c:470/471`.
5. More importantly, **the same defect occurs with the unchanged, valid Aeaea configuration** when audit wrappers fail project allocation 18, 22, 26, or 30. Failure 18 also produces two invalid frees under Valgrind.

The statement allows assuming correct configuration syntax. Therefore, malformed-input tolerance alone would not be a sufficient reason to call this a mandatory failure. The valid-input allocation-failure reproduction makes this directly relevant to the mandatory allocation-error and memory-safety requirements on p. 19.

**Required correction:** clear temporary ownership after a successful append, or parse each new route into a freshly initialized temporary record. Every error branch must free only allocations owned by that iteration. Re-test both malformed later routes and tokenization allocation failure after one or more successful routes.

Evidence: `audit-work/bad-route-valgrind.log`, `audit-work/valid-config-oom-valgrind.log`, `audit-work/probe-results.json`.

### F2 — P2: Allocation failures are incorrectly reported as unknown commands

Locations: [commands.c:247](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/src/commands.c:247>), lines 247–253; also argument-copy failure at lines 138–141 and 178–179.

`parseCommand`, `classifySail`, and `classifyBuySell` return `PARSE_UNKNOWN` when an allocation fails. That status means an unrecognized command, so a valid command is misdiagnosed and the session can finish with success status.

**Reproduced:** with input `MAP\n`, failing project allocation 26 or 27 prints `Unknown command` and exits 0. With `SAIL Aeaea\n`, failures 26–28 have the same result. stderr is empty.

**Required correction:** introduce a separate parser resource-error result. Propagate it to the terminal, issue an allocation-failure diagnostic using a path that does not require further heap allocation, and either recover explicitly or cleanly terminate with failure. Keep `PARSE_UNKNOWN` exclusively for an unknown verb. Update API documentation to describe the additional result.

Evidence: `audit-work/probe-results.json`.

### F3 — P2: A failed allocation at EOF silently drops the last command

Location: [Odysseus.c:150](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/src/Odysseus.c:150>), lines 150–157; caller at 201–203.

`lineBufferTakeRemainder` returns `NOSTOS_ERROR` when allocating the final unterminated line fails. `handleEofRemainder` only handles `LINE_FOUND`, returns `void`, and hides the error. The terminal then exits with its unchanged success status.

**Reproduced:** input `MAP` without a trailing newline, with project allocation 25 failed, produces only the startup message and prompt, no result or diagnostic, and exit status 0. The same allocation failure on newline-terminated input returns 2, demonstrating the inconsistent EOF path.

**Required correction:** make EOF remainder handling return a status, distinguish empty remainder from failure, and propagate failure through cleanup. Add a regression for allocation failure specifically while extracting an EOF remainder.

Evidence: `audit-work/probe-results.json`.

### F4 — P2: Terminal output failures are discarded

Locations: [Odysseus.c:80](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/src/Odysseus.c:80>), lines 80–85; unchecked prompts at 113 and 176. Related unchecked shutdown-message writes occur in `Ithaca.c` and `Island.c`.

The low-level `writeAll` correctly detects write failures, but `executeAndPrint` discards every returned status. The terminal also discards prompt-writing errors. Thus the presence of a checked I/O helper does not mean errors are handled end to end.

**Reproduced:** an audit wrapper permits the readiness message and first prompt, then returns `ENOSPC` for subsequent stdout writes. Input `MAP\n` produces no command response, no stderr diagnostic, and exit status 0.

There is a related diagnostic gap: terminal read/line-buffer failures can reach exit status 2 without informing the user. For example, the newline-terminated allocation-25 case exits 2 with empty stderr. This conflicts with p. 19’s requirement to inform the user of I/O/allocation errors.

**Required correction:** return and propagate command-output and prompt statuses. On terminal failure, attempt a simple diagnostic on stderr, release resources, and return failure. Check shutdown-message results consistently as well. A failed diagnostic must not trigger an endless retry or prevent cleanup.

Evidence: `audit-work/probe-results.json`.

### F5 — P2: The code does not yet meet the supplied strict style guide

This finding concerns the user's explicit course-style constraint, not a preference for a different C style.

| Rule | Confirmed deviation | Correction |
| --- | --- | --- |
| System includes belong in the module's own header when one is used (p. 5) | Eight runtime sources directly include system headers: `commands.c`, `config.c`, `io.c`, `lifecycle.c`, `routes.c`, `stock.c`, `text.c`, `voyages.c`. Example: [commands.c:11](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/src/commands.c:11>). | Move necessary system includes into the appropriate own headers; retain guards and the required header ordering. |
| Each function **definition** needs a descriptive name/purpose/argument/return header (pp. 13, 18) | 35 source comments use `@Def: See <header>.`; many omit `@Arg` and `@Ret` entirely. Example: [lifecycle.c:22](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/src/lifecycle.c:22>). Full prototype documentation elsewhere does not fulfill the literal definition-header rule. | Add the required descriptive fields above each definition, including argument direction, meaning, and possible return values. |
| Course Hungarian prefixes (p. 10) | `int bIsVoyages`, `int bIsMarket`, `int bEof`, and `int bHasNumber` use `b` instead of the prescribed `n`. Example: [types.h:141](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/include/types.h:141>). | Apply the prescribed integer prefix consistently, including pointer parameters that refer to these variables. Preserve vendor/system identifiers. |
| Constant-first comparisons (p. 15) | Examples include `lValue < 1`, `lValue > INT_MAX` at [commands.c:108](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/src/commands.c:108>), and corresponding amount/port/stock comparisons. | Apply constant-first ordering wherever one operand is a constant. Do not mechanically reverse variable-versus-variable comparisons. |

Positive style results: runtime functions fit the 45-physical-line limit; the modules are sensibly separated; project headers have guards; ordinary indentation and brace placement are consistent; no mutable shared runtime globals were found.

The statements in `docs/audit-handoff.md` around lines 98–101 and its completed style checkbox are therefore overstated. Update those claims after performing a complete style pass.

### F6 — P2: The supplied Odysseus “SIGINT” memory test normally tests EOF

Location: [run_memcheck.sh:61](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/tests/run_memcheck.sh:61>), plus `run_sigint_case` at lines 25–32.

The Odysseus case redirects stdin from `/dev/null`. Odysseus can immediately reach EOF and exit before the script's one-second delay ends. The later `kill -INT` failure is suppressed and not checked. A clean EOF run can consequently be labeled as the SIGINT case.

Other limitations: the script does not assert the expected process exit status, has no bounded completion check, and its `check_log` function claims to check non-inherited open descriptors but does not actually inspect descriptor entries. Missing Valgrind prints a skip and returns success, so exit status alone cannot certify memory-test coverage.

**Required correction:** keep Odysseus stdin open, wait for readiness, send partial input if testing the accumulator, verify the process is still running, deliver SIGINT, bound the wait, and assert clean exit. Parse both memory and application-owned descriptor results. Distinguish skipped checks from passed checks.

This is a defect in the supplied test evidence, **not evidence that ordinary Ctrl+C handling is broken**. Independent terminal-generated Ctrl+C and buffered partial-input SIGINT tests passed under Valgrind during this audit.

### F7 — P2: `make test` does not rebuild its loader checker and can silently lose coverage

Locations: [Makefile:63](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/Makefile:63>) and [run_functional_tests.sh:183](<C:/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos/tests/run_functional_tests.sh:183>).

`make test` depends on the three application binaries, but has no build rule/dependency for `tests/loader_check`. It runs the prebuilt checker if available. Changes to loaders can therefore be tested through an old checker binary. If that binary is absent, eight loader checks are skipped and the target still succeeds.

**Reproduced:** after temporarily moving the checker aside in the audit copy, `make test` returned 0 with `52 passed, 0 failed` and a skip message. After explicitly rebuilding the checker against the current objects, the suite returned `60 passed, 0 failed`.

Also, the script's “field-level” assertions count output lines/products/voyages rather than compare field values. Those assertions alone would miss incorrect names, amounts, prices, or endpoints. The independent audit did compare these values and they passed.

**Required correction:** make the checker a source-dependent target required by `make test`; ensure clean removes generated checker artifacts; treat required missing coverage as failure; compare actual loaded fields and filtered endpoints against expected data. Check process statuses as well as output text.

## 3. Independently verified results

Environment: WSL2 Ubuntu, GCC 13.3.0, kernel `6.18.33.2-microsoft-standard-WSL2`, Valgrind 3.22.0. This is a real Linux build and run, but not a Montserrat certification.

| Check | Result |
| --- | --- |
| Clean build of all three executables using submitted Makefile | Passed; no GCC compiler warnings. `make` emitted small Windows-mounted-filesystem timestamp/clock-skew warnings; all three binaries were rebuilt. |
| Supplied functional suite, with independently rebuilt checker | 60 passed, 0 failed. |
| Supplied six-case memory script | Reported all clean; coverage limitations are F6. |
| All 24 voyage records | Object name, path, destination, reward, and generated sequential IDs matched input. |
| All six stock files / 48 records | Every product name, amount, and price matched an independent Python binary decode. |
| Six islands' Sphragis-filtered routes | Correct observed survivor names and original IP/port endpoints; counts 2, 1, 1, 1, 3, 2 for Aeaea, Aeolia, Ismarus, Ogygia, Scheria, Thrinacia. |
| Original Sphragis object | SHA-256 comparison confirmed the submitted `.o` is identical to the supplied vendor object. |
| Odysseus loaded identity and resources | Name, gold, food count/entries, Ithaca endpoint, and initial island/endpoint checked successfully. |
| CRLF commands, final line without newline, 100,000-character island argument, 1,000-command batch | Passed under ordinary allocation conditions. |
| Terminal-generated Ctrl+C for all three processes | Passed using a controlling pseudo-terminal under Valgrind: exit 0, zero memory errors, zero bytes in use at exit. Only Valgrind's inherited log descriptor remained beyond standard descriptors. |
| SIGINT with a partial command already delivered through a pipe | Passed under Valgrind; partial command discarded and cleanup completed. |
| Idle CPU | Each process accrued 0 CPU ticks over its one-second observation interval. |
| Prohibited runtime operations | No prohibited stdio I/O, shell execution/stat calls, sockets, forks, or threads found in the authored runtime source scan and review. `vasprintf` is permitted formatting; vendor internals are not student-authored calls. |
| Allocation/write failure injection | Confirmed F1–F4. These failures are not covered by the ordinary 60-pass result. |

The `signalfd`/`poll` approach is supported by the course UNIX material. It is not a style violation merely because it is more elaborate than a short signal example. Likewise, syntax-only commands, acceptance of unknown island/product names, and absence of trading/network functionality are correct for this phase.

## 4. Remaining delivery and interpretation issues

These are separate from the confirmed runtime bugs.

1. **Montserrat remains unverified.** The statement names it as the target. Run a clean build, ordinary cases, Ctrl+C checks, and memory tests there after fixes. No remote credentials or validation were assumed.
2. **The supplied handoff is a ZIP/directory, not the final required `G<group>_F1.tar`.** This is acceptable for asking for an audit, but the same ZIP should not be uploaded as the official delivery. `make package` currently only builds and prints advice; it does not create a submission archive. Assemble the real tar, list it, extract it into an empty directory, and build/run all three executables there.
3. **Report information remains pending.** `docs/report.md` is a substantive progress report, but group information and actual hours are unfinished. Supply real values; do not fabricate work history. The statement explicitly requires final PDF format for the final delivery, so lack of a final PDF alone is not being labeled a Phase 1 code failure.
4. **Instructor validation is not established.** The documents acknowledge this. The source's p. 20 recommendation and p. 38 mandatory overall-design-validation wording should be resolved with the course staff; this audit is not that approval.
5. **Keep interpretation choices distinct from requirements.** The precise numeric grammar/ID limits, blank-line behavior, `BUY MAP 2`, and singleton-Ithaca enforcement are documented assumptions. I did not relabel them as proven bugs without an authoritative ruling.
6. **The test-helper exemption is self-declared.** `tests/loader_check.c` uses `printf`/`fprintf` and relaxed style. It is not linked into the three graded executables, so the runtime passes the prohibited-I/O scan. However, neither the helper's comment nor the handoff document grants instructor approval for forbidden APIs in a submitted C test file. Before packaging, either bring that C helper into compliance or explicitly keep it outside the graded source archive if course submission policy permits that. Preserve an auditable test procedure separately.
7. **Correct the documentation after fixes.** Replace blanket assertions of complete style/error-path coverage with actual checked results. Preserve the distinction between local tests and Montserrat tests, between EOF and SIGINT, and between historical test claims and new runs.

## 5. Correction and re-audit order

1. Fix F1 ownership and add a regression for allocation failure after a stored route.
2. Fix F2 parser error classification, F3 EOF status propagation, and F4 output/error diagnostics.
3. Apply F5 across all authored runtime files, preserving the working modular structure and function-length limit.
4. Repair F6/F7 so the tests validate the current source and the intended shutdown paths.
5. Re-run ordinary command/data tests, targeted regressions, and Valgrind. A zero-leak summary alone is insufficient: F1 showed invalid frees while still leaving zero bytes allocated at exit.
6. Update the progress report and verified-test records, check Montserrat, and prepare the actual tar archive.

There is no need to rewrite the entire project. The normal-path loaders, binary decoding, route filtering, command grammar, and ordinary signal lifecycle provide a working base. The immediate work is to make ownership and error propagation correct, then enforce the required style and make the tests trustworthy.

## 6. Reproduction evidence

All paths below are relative to the workspace root, outside the original submission:

- `audit-work/probe.py` and `audit-work/faults.c`: build separate audit binaries with linker wrappers and reproduce F1–F4. The wrappers inject failure into project calls; they do not replace Sphragis or change its topology.
- `audit-work/probe-results.json`: exact observed stdout, stderr, statuses, and injected allocation indices.
- `audit-work/verify.py` and `audit-work/verification-results.json`: independent stock/voyage/route comparisons, long/multiple-line cases, pseudo-terminal Ctrl+C, and valid-config OOM regression.
- `audit-work/final_checks.py` and `audit-work/final-checks.json`: idle CPU, buffered partial-input SIGINT, current-checker suite, and missing-checker coverage reproduction.
- `audit-work/bad-route-valgrind.log` and `audit-work/valid-config-oom-valgrind.log`: confirmed double-free traces.
- `audit-work/*-pty-valgrind.log` and `audit-work/odysseus-buffered-sigint.log`: independent clean shutdown evidence.
- `audit-work/functional-rerun.log` and `audit-work/missing-checker.log`: 60-pass and 52-pass suite outcomes.
- `audit-work/submitted-sha256.json`: fingerprints of the reviewed submission files.

To reproduce the audit probes in Linux, from this workspace run `python3 audit-work/probe.py`, followed by `python3 audit-work/verify.py` and `python3 audit-work/final_checks.py`. They expect the existing audit copy, GCC, Python 3, and Valgrind. The audit copy was created from the submitted directory and built with `make clean` followed by `make all`. Allocation indices refer to these exact files and wrapper setup; after corrections, re-target fault sites rather than assuming those numbers remain unchanged.
