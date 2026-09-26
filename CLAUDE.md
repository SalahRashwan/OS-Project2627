# Claude Code context: The Nostos System, Phase 1

## Read this first

The user wants a course-compliant implementation of **Phase 1: Inithaca**, followed by an independent audit by Codex. This folder currently contains teaching materials and a planning handoff, not an implemented project. Implement Phase 1 only unless the user explicitly expands the scope. Later-phase material is architectural context, not permission to implement it now.

Read `PHASE1_IMPLEMENTATION_GUIDE.md` completely before coding. It contains the ordered implementation plan, source references, acceptance tests, and unresolved questions. Keep this file as the persistent context when the conversation is compacted.

The original material directories are:

- `Phase1/`: the statement, style guide, rubric, official tests, assigned ports, Sphragis library, and fixtures.
- `All Materials/`: course notes, textbooks, tools guidance, scheduling exercises, and examples.
- The actual sample directory is spelled **`All Materials/Codeing samples/`**.
- The actual library directory is spelled **`Phase1/Sphragis libray/`**.

Preserve originals. Put implementation in `nostos/` unless the user chooses another location. Copy required fixtures/library into that implementation so its submission archive is self-contained. Do not edit the teaching examples or repair the supplied binary library.

## Authority and course methodology

1. Follow the user's explicit scope and the current Nostos 2026-27 statement.
2. Apply `Phase1/Style Guide.pdf` to all student-authored code.
3. Use the supplied Phase 1 test document and rubric as acceptance/evaluation guidance.
4. Use the **actual `sphragis.h`** for library signatures and ownership contracts.
5. Use the course coding samples and course explanations as the implementation methodology. Prefer simple C, file descriptors, explicit data structures, small functions, and clear ownership.
6. Where a teaching example contradicts an explicit project rule, follow the project rule and explain the adaptation. Do not copy an example's deliberate race, unchecked allocation, overflow, or prohibited I/O function.

The tests filename says 2025-26 but its contents explicitly describe Nostos 2026-27. The rubric has an older filename and generic assessment categories. Do not implement the previous year's Citadel project.

## Non-negotiable scope

Build these three executables using a Makefile:

```text
./odysseus <config.dat>
./ithaca <config.dat> <voyages.dat>
./island <config.dat> <stock.db>
```

Use entry-point files `Odysseus.c`, `Ithaca.c`, and `Island.c`, plus reusable `.c`/`.h` modules. One `island` executable handles every island by configuration.

Phase 1 must:

- Load and retain every specified field of all three configuration formats.
- Load Ithaca's voyage records, assigning identifiers internally.
- Load each island's binary stock records.
- Call `SPHRAGIS_filter_island_configuration()` and retain only its surviving routes, with the matching IP/port data.
- Give **only Odysseus** an interactive terminal.
- Recognize commands case-insensitively, validate syntax/numbers, and extract arguments.
- Print exactly `Command OK` for valid commands and `Unknown command` for unknown commands. Recognized commands with invalid syntax print their usage.
- Keep Ithaca and Island alive, blocked without busy waiting, until CTRL+C.
- Terminate all three cleanly on CTRL+C and release owned memory/descriptors, including on initialization failures.

Do not add sockets, network connections, worker threads, forks, IPC queues, shared memory, semaphores, trading, food timers, file transfers, glyph signing, mission state changes, or persistence. `MAP` and `STATUS` also only print `Command OK`. Do not display a real map in Phase 1. Signal handling is required despite the ban on concurrency mechanisms.

## Mandatory C and style rules

- Screen/file data I/O: `read()` and `write()`; file lifecycle: `open()` and `close()`.
- No `printf`, `fprintf`, `scanf`, `fscanf`, `gets`, `puts`, `getchar`, `fgets`, `fopen`, `fread`, `fwrite`, `getline`, `perror`, or other stdio I/O shortcuts in authored runtime code.
- `asprintf`, `snprintf`, and similar **in-memory formatting** are permitted. Follow the course's format -> write -> free pattern and check failures.
- No `system()`, `popen()`, `stat()`, `fstat()`, `lstat()`, or variants. No shelling out to parse inputs or inspect their sizes.
- Compile with at least `-Wall -Wextra`; no warnings. A GNU C dialect is appropriate for the provided GNU/Linux examples. Enable `_GNU_SOURCE` consistently before system headers when using `asprintf`.
- Hungarian variable names: `nCount`, `lValue`, `psName`, `sName[100]`, `pnValues`, `stConfig`, typedefs beginning with `t`, enums with `e`; unavoidable globals also get `g`.
- Functions: descriptive lowerCamelCase; keep each definition within 45 lines, conservatively counted from signature through closing brace.
- Four spaces per indentation level (Makefile recipes require real tabs). Same-line opening brace with preceding space. Spaces around operators and after commas.
- File headers: `@File`, `@Purpose`, `@Author`, `@Date`. Function headers: `@Name`, `@Def`, `@Arg` with input/output meanings, and `@Ret`.
- Constants/macros uppercase with underscores. Constant-first comparisons where applicable. No chained assignments; prefer `if` over ternary; avoid `goto`; every switch has `default` and explicit `break` in every branch.
- Comments go above the relevant code. Initialize variables and use narrow scope; place function locals at the beginning of the function as the guide requests.
- When using project headers, put needed system includes in those headers and include project headers from `.c` files. Use include guards. Do not define mutable globals in shared headers/modules.
- Treat external library names/fields and system-defined structures as fixed APIs; never rename them to satisfy Hungarian notation.

The supplied Sphragis object itself imports stdio functions. This does not authorize their use in student code. Preserve the required vendor binary and audit authored source/objects separately.

## Commands

| Form | Phase 1 rule |
| --- | --- |
| `CONNECT ITHACA` | Exactly these two command words |
| `LIST VOYAGES` | Exactly these two command words |
| `ACCEPT <voyage_id>` | One valid positive decimal integer under the documented ID policy |
| `SAIL <island>` | One nonempty token; no reachability/state check |
| `MAP` | No arguments |
| `LIST MARKET` | Exactly these two command words |
| `BUY <product> <amount>` | Product token and positive integer quantity; `BUY MAP 1` supported |
| `SELL <product> <amount>` | Product token and positive integer quantity |
| `STATUS` | No arguments |
| `DELIVER` | No arguments |
| `CLAIM` | No arguments |

Reject extra tokens, numeric junk, overflow, zero/negative quantities, and missing arguments. Preserve argument spelling; compare command words without case. Never reject `ACCEPT 999` because it is absent from a voyage list, or `BUY DriedFish 3` because no connection exists. State validation belongs to Phase 2.

Certain edge policies are not fully specified: blank lines, signs/leading zeros in integers, `LIST nonsense`, and Phase 1 treatment of `BUY MAP 2`. Use the explicit defaults and ambiguity register in the guide, with tests. Do not present these choices as verbatim instructor requirements.

## Data and ownership traps

- The supplied `voyages.dat` contains **24** records, with four fields each and no ID/count header.
- The six supplied `.db` files are binary, each **864 bytes**, holding **8 records of 108 bytes** on the observed layout: 100 name bytes, a 32-bit little-endian amount, then a 32-bit little-endian price. Verify compatibility on Montserrat; do not replace the format with text or pointer-containing structs.
- Do not hardcode 24, 8, routes, prices, quantities, or voyage contents as runtime results.
- `SPHRAGIS_filter_island_configuration()` frees rejected destination names. Supply separately heap-allocated names. Use an independent temporary name array so original route records do not acquire dangling names. Match survivors back to the original route records, then free each owned allocation once. See the detailed adapter algorithm in the guide.
- The library function returns the **new nonnegative route count**, not `SPHRAGIS_OK` as its only success value. Negative values are errors.
- After filtering, library names alone are insufficient: retain corresponding route IP addresses and ports.
- The PNG images are later-phase assets; retain their paths in voyages now, but do not open/transfer/sign them.
- Relative path policy must be documented. Suggested fixture layout uses `objects/` relative to the runtime working directory, matching supplied voyage paths. Do not prepend `objects/` twice. Linux case matters: the original asset folder is `Objects`, while voyage paths use `objects`.
- Every owned aggregate has initialization/destruction functions safe for partial initialization. Counts include only fully owned entries.
- A failed `realloc` must not overwrite the only owner. A failed `asprintf` does not provide a usable/freeable output pointer.
- Never assume one `read` equals one complete line/record. Distinguish EOF, error, interruption, and partial data.

## Runtime and shutdown

The target is the university GNU/Linux environment, with final validation on **Montserrat**. The supplied object is ELF64 little-endian x86-64, not a native Windows object. WSL/local Linux can help, but is not proof of Montserrat acceptance.

Shutdown must not depend on another Enter press or a second CTRL+C. Do not allocate, format dynamically, or free application state in a signal handler. The course UNIX book explains `sigaction`, flag-only handlers, signal masks, `sigsuspend`, and `signalfd`. The guide offers a race-free, course-backed design using blocked SIGINT plus `signalfd` and `poll`, without threads. Equivalent safe designs may be used if documented and tested. Do not use a spinning flag loop or a race-prone check-then-`pause` loop.

The ports document lists Salah Ahmed Salaheldin Adly Rashwan's range as **8620-8624**. These are configuration values only in Phase 1. Group number, teammates, actual deployment IPs, and design-validation status are not established by this handoff. Do not invent them or infer the submission group from the port range.

## Instructor/design and submission obligations

The main statement PDF page 38 explicitly makes overall design validation with instructors/interns mandatory before Phase 1. PDF page 20 describes a pre-phase design document as nonmandatory and recommends validation. Record the distinction/conflict and obtain actual validation status from the student; never claim an approval happened. You can prepare the design and implementation artifacts for review without fabricating validation.

The supplied schedule gives Phase 1 as **4 October 2026**. This is a transcription of the local document, not a verified live eStudy deadline.

Submit a genuine `G<actual_group>_F1.tar` containing source, headers, Makefile, required Sphragis files, configuration/data files, and the evolving report. The final report must be PDF; Phase 1 already needs a progress report. Do not upload/submit on the user's behalf without an explicit request.

## Completion contract

Deliver the implementation with:

1. `make` producing all three required executables from a clean directory.
2. Reproducible tests for every command, all loaders, filtered routes, errors, EOF, and CTRL+C.
3. Valgrind checks for each executable and representative failure paths, with file-descriptor tracking.
4. README run instructions, module/ownership design, progressive report, and explicit assumptions.
5. A test-results file recording exact commands, platform/compiler, actual results, and unrun checks.
6. An audit handoff mapping the guide's requirements to files/functions and test evidence.

Do not claim a test passed unless you ran it. Do not suppress compiler warnings or mock Sphragis in the delivered build. If Montserrat access is unavailable, finish local work and mark remote verification pending. The final response must distinguish implementation completion from pending human validation or remote checks.
