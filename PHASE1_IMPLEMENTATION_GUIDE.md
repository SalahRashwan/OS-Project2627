# The Nostos System: detailed Phase 1 implementation and audit guide

Prepared from the supplied local materials on 21 September 2026.

## 0. Purpose, scope, and how to use this handoff

This guide tells Claude Code how to build **Phase 1: Inithaca** using this course's C methodology, and tells the later reviewer what evidence to inspect. It is a plan, not a claim that the project has already been implemented or tested.

Read `CLAUDE.md`, then this entire guide, then the cited original sources. Keep both original material folders available to Claude. The two Markdown files do not replace the specification or the actual library header.

The directory name `Phase1` and the supplied testing sheet establish the working scope. The main statement also contains Phases 2 and 3. Read those to avoid architectural dead ends, but do not implement those phases in a Phase 1 submission. If the user later requests all phases, prepare an additional detailed plan for those phases before extending the code.

Throughout this guide:

- **Required** means an explicit project/style requirement or a direct necessary consequence of it.
- **Recommended design** means a concrete implementation choice selected for this handoff. It can be changed if the replacement is equally course-compliant and documented.
- **Open/interpretation** means the materials do not fully determine the behavior. Keep the assumption visible and easy to change.
- **Future only** means architectural context, not current work.

Use PDF viewer page numbers below. The main statement's printed page is usually one lower than its PDF page. The UNIX book's Arabic printed page is generally ten lower than its PDF page.

### Completion sequence

1. Establish source requirements and record open questions.
2. Prepare module and ownership design for instructor validation.
3. Establish the Linux build and real Sphragis linkage.
4. Implement reusable I/O, parsing, allocation, and cleanup.
5. Implement all configuration/data loaders and route filtering.
6. Implement the Odysseus command parser without command functionality.
7. Implement clean process lifecycles and interruption handling.
8. Run functional, failure, memory, and style checks.
9. Produce the progress report and reproducible audit evidence.
10. Verify a self-contained tar archive from a fresh extraction on Montserrat.

Do not begin by writing a large main function and splitting it afterward. Define small responsibilities and destructors first.

## 1. Sources and precedence

### 1.1 Project sources

| Reference | Exact local source | Relevant content |
| --- | --- | --- |
| P | `Phase1/OS_Project_2026-27_ICE_cleaned.pdf` | 59 PDF pages: full Nostos statement |
| P1 | P, PDF pp. 11-20 | Phase 1 formats, commands, process lifecycle, restrictions |
| P2 | P, PDF pp. 21-32 | Future networking, ports, markets, maps, concurrency |
| P3 | P, PDF pp. 33-37 | Future transfers, glyphs, persistence |
| D | P, PDF pp. 38-40 | Design validation, schedule, archive, report requirements |
| W | P, PDF pp. 41-56 | Future wire protocol |
| A | P, PDF pp. 57-59 | Modular design and concurrency guidance |
| S | `Phase1/Style Guide.pdf` | 23 PDF pages; mandatory student-code conventions |
| T | `Phase1/OS_Project_2025-26 - testing Phase 1.pdf` | 2 pages; contents are Nostos 2026-27 despite filename |
| R | `Phase1/Rubric-2025-26.pdf` | Execution 5, documentation 2, interview 3 |
| L | `Phase1/Sphragis libray/sphragis.h` | Exact public API and memory-ownership contract |
| O | `Phase1/Sphragis libray/sphragis.o` | Required precompiled implementation |
| PL | `Phase1/Ports List.pdf` | Student port allocations |

Resolve disagreements by reading explicit rules before illustrative examples. The examples are not complete implementations. The style guide itself includes `printf` examples, but the project forbids that I/O. Several lab samples contain simplified or intentionally unsafe code. Preserve the taught mechanism, not the defects.

The user specifically requires the coding examples and course explanations to govern methodology. Do not replace this assignment with a generic framework, web app, C++ project, Python implementation, parser generator, or third-party networking library.

### 1.2 Course material map

The inventory was reviewed across project documents, course notes, examples, support documents, workbook, binary fixtures, and image assets. Implementation-relevant sections were examined in detail; broad theory and advanced UNIX topics were reviewed for context and applicability. This is not a claim that every unrelated textbook example was executed or independently proven correct.

| Material | What it contributes | Phase 1 use |
| --- | --- | --- |
| `Chapter 1 - Introduction to Operating Systems and the Kernel (2).pdf`, 16 pp. | CPU/memory/I/O, interrupts, kernel layers, process states, dispatcher/context switching | Explain why blocking waits avoid wasting CPU and why applications use system calls |
| `Tema2-Scheduling (2).pdf`, 12 pp. | FCFS, SJF/SPN/SRT, priorities, RR, HRRN, response/wait/turnaround metrics and simulator | Conceptual context; do not implement a scheduler |
| `OS_Chapter2-Problems2026-2027 (1).pdf`, 26 pp. | Scheduling exercises and interrupt-order conventions | Coursework context, not Nostos acceptance tests |
| `Scheduling (1).xlsx` | Two sheets, `Problema 1` and `Problema 2`: RR quantum 3 and SRT exercises | No runtime dependency; no need to solve/import these into Nostos |
| `Chapter3-Concurrency-MutualExclusion-Mutex-EN-v1.pdf`, 16 pp. | Critical sections, lost updates, mutex ownership, deadlock/starvation/fairness | Future design context; no Phase 1 mutexes |
| `Chapter3-Synchronization-Semaphores-EN-v1.pdf`, 18 pp. | Permits/events, initial values, course SEM API, creator/consumer lifecycle | Future design context; no Phase 1 semaphores |
| `Chapter4.0.Entorn de Treball.pdf`, 8 pp. | University hosts, SFTP/SSH, GCC warnings, Valgrind | Build/test environment |
| `Chapter4.FileDescriptors.pdf`, 3 pp. | Replace stdio with descriptors, `open/close/read/write`, format in memory | Central implementation rule |
| `Chapter4.Forks.pdf`, 4 pp. | Fork return branches, memory copies, wait, exec | Future only |
| `Chapter4.Pipes.pdf`, 6 pp. | Byte streams, close unused ends, dup/dup2, exec | Future MD5 subprocess design only |
| `Chapter4.Signals.pdf`, 6 pp. | SIGINT, handlers, pause, alarms | Required CTRL+C concept, adapted safely |
| `Chapter4.Threads.pdf`, 12 pp. | Thread creation/join, argument/result lifetime, shared state | Future only |
| `Chapter4.MemCompartida.pdf`, 4 pp. | shmget/shmat/shmdt/shmctl lifecycle | Future local IPC context, not Phase 1 |
| `Chapter4.CuesMissatges.pdf`, 15 pp. | Message passing, buffering, System V queue API | Future local IPC context, not Phase 1 |
| `Chapter4.Select.pdf`, 4 pp. | Descriptor sets and readiness multiplexing without parallel execution | Conceptual basis for a blocking terminal/signal wait |
| `Other materials/Introduction-C-Programming-Language-2026-27-en.pdf`, 37 pp. | Types, control flow, arrays/strings/structs, functions, files, heap ownership | Apply C/memory rules; its stdio chapter does not override the OS I/O restriction |
| `Other materials/UNIX-Programming-2026-27-en.pdf`, 301 pp. | Processes, basic I/O, output idiom, signals/timers, IPC, sockets, threads, advanced I/O/signals/networking, string appendix | Main explanatory reference; use sections listed below |
| `Other materials/Valgrind_eng.pdf`, 11 pp. | Memcheck, leak classes, invalid access/free, origins, descriptor tracking | Verification method |
| `Other materials/VSCode Guide.pdf`, 6 pp. | Local versus remote files, SFTP setup, upload/synchronization | Workflow context; older 2023-24 screenshots |
| `Other materials/vpn_help.txt` | University VPN documentation pointers for Mac/Windows/Linux | Access guidance only; no credentials supplied |

The workbook was inspected read-only, including its cell contents. The 24 supplied PNGs were checked as readable images and reviewed in a contact sheet. They represent mission objects and add no Phase 1 algorithmic requirements. The `.o` was inspected as an object file, not executed or decompiled to infer hidden topology.

### 1.3 UNIX book reading locations

- Chapter 2, especially PDF pp. 31-35: descriptor semantics, relative paths, open/close/read/write, return values and partial transfers.
- Chapter 3, PDF pp. 51-56: stdio buffering pitfalls, the `asprintf` -> `write` -> `free` idiom, failure handling, and flag-only signal handlers.
- Chapter 4, PDF pp. 57-68: signal dispositions, masks, handlers, and reentrancy.
- Chapter 12, especially sections 12.2-12.6, PDF pp. 213-225: `sigaction`, `EINTR`/`SA_RESTART`, `sigsuspend` race avoidance, `sigwait`, and `signalfd`.
- Appendix A, PDF pp. 283-297: character conversion, `strcasecmp`, string/buffer distinction, formatting, `strtok`/`strtok_r`, and explicit byte lengths.
- Chapters 1 and 5-9 explain processes, timers, pipes/queues, shared memory/semaphores, sockets, and threads for later phases.
- Chapters 10-14 cover advanced processes, I/O, signal handling, socket options/nonblocking sockets, and Ethernet/raw interfaces. Their availability is not a requirement to use them. `stat`, mmap-based file loading, UDP, raw sockets, and custom process cloning are not appropriate ways to implement Phase 1.

### 1.4 Coding samples: exact lessons and limitations

| Sample group/files under `All Materials/Codeing samples/` | Follow this lesson | Do not copy this limitation |
| --- | --- | --- |
| `File Descriptors/file_descriptors_1.c`, `_2.c`, `_3.c` | Descriptor input/output; dynamic formatting and freeing; `_2` adds open/close and stderr | Unchecked read results and blindly writing `string[n-1]` |
| `File Descriptors/class_solutions.c` | Short explicit parsing exercises, read/write and small functions | Raw binary int input for human text, `atoi/atof` as complete validation, fixed-buffer edge errors |
| `Sockets/client.c`, `server.c` | `readUntil` concept: delimiter-driven dynamic string accumulation; later socket lifecycle | Its exact `readUntil` can spin at EOF and loses the original pointer on realloc failure; server has an oversized read into an inadequate buffer |
| `Signals/signals_1.c`, `_2.c`, `_3.c` | Signal registration and a blocked process waking on signal | Endless demonstration loops, formatting/allocation in handlers, default re-raise before application cleanup, and `printf` |
| `Forks, exec/forking.c`, `forking1.c` through `forking5.c` | Parent/child distinction, private memory, inherited descriptors, exec and wait | Importing forks into Phase 1; unhandled failure paths or allocations inside alarm handlers |
| `Pipes/ex1.c` through `ex5.c` | Pipe ends, close discipline, dup2 and exec; `ex4` is useful future MD5 inspiration | stdio output, missing checks, fixed-size reads as message boundaries, waiting before draining a potentially full pipe |
| `Threads/example1.c` through `example4.c` | Create/join, per-thread argument lifetime, heap result ownership, race demonstration | `example3`'s intentionally unprotected counter or any Phase 1 threads |
| `Threads/slides_example1.c` through `slides_example4.c` | Shared memory versus fork, parameter passing and join behavior | Infinite output loops, unprotected global updates, inconsistent naming/style |
| `Select/poll.c`, `epoll.c`, `select.c`, `server.c` | Blocking readiness, rebuilding select sets, later multi-client structure | Unchecked reads, nonterminated buffers, out-of-bounds terminators, repeated thread dispatch for one descriptor, periodic status spam |
| `Semaphores/example1.c` through `example3.c`, `semaphore_v2.h` | Constructor/init/wait/signal/destructor; owner controls lifecycle | Reinitializing an existing semaphore, ignoring return values, treating wrapper naming as student-style precedent |
| `Shared memory/ex1.1.c`, `ex1.2.c` | Creator/attacher ownership, correct `-1` failure checks, detach/remove distinction | Assuming ID zero is invalid |
| `Shared memory/ex2.1.c`, `ex2.2.c` | Example of coordination concerns | Busy waiting, mismatched shared struct layouts, unsafe stdio patterns |
| `Shared memory/memory_cleaner_v2.sh` | Kernel IPC can outlive a process | Running a broad cleanup on university resources; not needed for Phase 1 |
| `Message queues/example.c`, `reader_example.c` | Positive leading long message type, payload size excludes type, checking errors, single cleanup owner | Adding a queue merely because the course teaches it |

For an implementation review, require a short mapping from each major module to the corresponding lesson. No invented claim that every source line must be copied verbatim.

## 2. Phase boundary and requirement checklist

### 2.1 Required current behavior

| ID | Requirement | Source | Evidence expected |
| --- | --- | --- | --- |
| F01 | All three named executables and CLI forms | P pp. 11, 17-20 | Clean make and CLI tests |
| F02 | All Odysseus config fields stored | P pp. 11-12 | Field-level loader checks |
| F03 | Ithaca config and voyage records stored; IDs internal | P pp. 12-13 | Fixture comparison and ID uniqueness |
| F04 | Island config and raw route endpoints loaded | P pp. 13-14 | Multiple configurations, endpoint checks |
| F05 | Real mandatory Sphragis filtering before valid use | P pp. 14, 18; L | Source call, real linked build, survivor/ownership tests |
| F06 | Binary stock records loaded until EOF | P p. 15 | All six stocks and truncated-record test |
| F07 | Appropriate persistent-in-process data structures | P pp. 18-19 | Ownership diagram and debugger/test assertions |
| F08 | Only Odysseus has an interactive terminal | P pp. 15-18 | Three runtime sessions |
| F09 | All eleven command forms recognized | P pp. 15-17; T p. 2 | Complete command test matrix |
| F10 | Case-insensitive words and numeric validation | P pp. 15-19; T p. 2 | Mixed case and numeric edge tests |
| F11 | Exact success/unknown messages and usage diagnostics | P pp. 16-18; T p. 2 | Captured output comparisons |
| F12 | No voyage/location semantic checks or command actions | P pp. 11, 15-16, 19 | Out-of-state valid commands still succeed |
| F13 | Ithaca/Island initialize, stay alive, then CTRL+C exit | P p. 17 | Lifecycle and idle-CPU test |
| F14 | All processes free resources on CTRL+C/errors | P p. 19 | Valgrind and descriptor checks |
| F15 | Required I/O and forbidden-API restrictions | P p. 19; FD slides | Source/own-object scan |
| F16 | Errors handled, no busy waits/crashes/warnings | P p. 19 | Failure tests, clean compiler output |
| F17 | Real Makefile and reusable `.c`/`.h` modules | P pp. 19-20; A | Build graph and source structure |
| F18 | Correct tar, included data/config/report, Montserrat | P pp. 38-40 | Fresh-extraction build/run and tar listing |
| F19 | Course coding conventions | S | Manual style audit |
| F20 | Overall design validation status honestly recorded | P p. 38 | Student/instructor evidence, never fabricated |

### 2.2 Explicitly excluded from Phase 1

No `socket`, `bind`, `listen`, `accept`, `connect`, request frames, TCP tests, market transactions, real map display, food-consumption timers, mission assignment state changes, file checksums/transfers, receipt signing, saved game state, worker threads/processes, or IPC synchronization objects.

The three executables are launched independently by the user. Do not create a launcher that forks them. Retaining loaded data in memory is required; writing it back to implement persistence is not.

The operating system may block the process for terminal/signal readiness. A single-threaded wait is not an implementation of concurrent voyage handling. Explain this distinction when documenting signal/readiness APIs.

## 3. Decisions and ambiguities to record before implementation

Create `nostos/docs/assumptions.md` and record each item. These are bounded questions, not permission to stall unrelated preparation.

| ID | Issue | Recommended default / required action |
| --- | --- | --- |
| A01 | Project scope not explicitly called “Phase 1 only” in the user's original prose | This handoff targets Phase 1 based on folder/test context; later scope changes require user direction |
| A02 | P p. 20 recommends validation, P p. 38 calls overall design validation mandatory | Preserve the distinction between a pre-phase document and overall design approval; ask student for actual validation status and prepare design for review |
| A03 | Only one Ithaca may execute at a time, but no singleton enforcement mechanism is specified | Run/test only one by default. Do not add sockets/semaphores or a fragile PID-file scheme. Ask instructor whether runtime enforcement is expected; mark pending if unknown |
| A04 | Voyage ID numbering policy not specified | Assign increasing positive IDs from 1 in file order during each load; stable within execution; never insert IDs into supplied input |
| A05 | Numeric syntax beyond “numerically valid” not fully defined | Accept digit-only decimal tokens, leading zeros allowed, value 1..INT_MAX for IDs/command quantities. Reject signs, decimal points, exponent notation, junk, and overflow. Mark exact lexical policy as a design choice |
| A06 | `BUY MAP 2` in syntax-only Phase 1 | Recommended: reject with BUY usage because Table 1 specifies `BUY MAP 1` and the map quantity is fixed. Do not check funds/location. If instructor treats quantity=1 as future semantics, change this one validator and test |
| A07 | `SELL MAP 1` versus later prohibition on selling maps | Accept its grammar in Phase 1; map trading semantics belong to Phase 2. No inventory/product membership checks |
| A08 | Empty/whitespace command | Ignore and reprompt, with no `Command OK`; document this UX choice |
| A09 | `LIST nonsense` or `CONNECT elsewhere` | Treat recognized first-word families as usage errors, not valid commands. For LIST show both supported forms. Only an unrecognized verb is Unknown command |
| A10 | Relative file paths | Store input paths unchanged. For run fixtures, interpret relative file paths from the working directory, as ordinary `open` does. No automatic double prefix of Ithaca folder and voyage file path |
| A11 | Figurative MAP output in Figure 9 | Follow explicit Phase 1 rule and official test: only `Command OK`; omit stars message and real map |
| A12 | No actual process config `.dat` files supplied | Create documented configs from statement formats; do not pretend they were provided. Use assigned ports for fixtures and clearly label sample IPs |
| A13 | Style header-global example versus Annex III | Prefer no mutable globals. If a handler flag is necessary, define once in its main process file; any needed header declaration uses `extern`, not a definition. Never put stateful globals in reusable modules |
| A14 | Sample naming versus Hungarian style guide | Apply guide to authored code, preserve vendor/system API names |
| A15 | Unknown authors/group/IPs/actual time spent | Use explicit pending fields in draft documentation and resolve before submission; do not invent teammate details, group 12, remote IPs, or historical work hours |
| A16 | Library integration documentation beyond `.h` absent | Use the real header/object, inspect link errors on Linux; do not invent missing helper functions or dependency flags |

These choices must remain visible to the later auditor. A reasonable interpretation is not an instructor-confirmed rule.

## 4. Step 1: create a clean project and establish the build

### 4.1 Working layout

Recommended layout, not a mandatory instructor filename list:

```text
nostos/
    Makefile
    README.md
    src/
        Odysseus.c
        Ithaca.c
        Island.c
        io.c
        text.c
        config.c
        voyages.c
        stock.c
        routes.c
        commands.c
        lifecycle.c
    include/
        Odysseus.h
        Ithaca.h
        Island.h
        io.h
        text.h
        config.h
        voyages.h
        stock.h
        routes.h
        commands.h
        lifecycle.h
        types.h
    lib/
        sphragis.h
        sphragis.o
    configs/
        odysseus.dat
        ithaca.dat
        aeaea.dat
        aeolia.dat
        ismarus.dat
        ogygia.dat
        scheria.dat
        thrinacia.dat
    data/
        voyages.dat
        stocks/<six supplied .db files>
    objects/<24 supplied PNG files>
    docs/
        design.md
        assumptions.md
        report.md
        test-results.md
        audit-handoff.md
    tests/
        <test driver, command fixtures, isolated failure fixtures>
    build/<generated student object files>
    odysseus
    ithaca
    island
```

Do not confuse these recommended modules with instructor-named mandatory modules. The explicit mandatory processes are the three entry points/executables; modularization is also mandatory. You may split `config.c` by process if it becomes too broad, or combine a genuinely tiny helper responsibility. Do not create dozens of empty future-phase modules.

### 4.2 Build tasks

1. Use a university Linux shell for authoritative compilation. The work-environment slides list Montserrat, Matagalls, and Puigpedros; the delivery statement specifically requires working on Montserrat.
2. Record `uname`, compiler version, and machine architecture in test evidence. Do not expose credentials or copy SFTP passwords into the archive.
3. Copy `sphragis.h` and `sphragis.o` unchanged into `lib/`.
4. Use `CC = gcc`, include paths for `include/` and `lib/`, and at least `-Wall -Wextra -g`. Recommended dialect: `-std=gnu11`.
5. Set `_GNU_SOURCE` once consistently, for example `CPPFLAGS += -D_GNU_SOURCE`. Avoid warning-producing repeated definitions.
6. Compile each `.c` independently; link only the necessary objects for each executable. Never `#include` a `.c` file.
7. Link Island with the actual `lib/sphragis.o`; Phase 1 Odysseus does not need real map display or image signing.
8. Confirm the object links on the target. Inspection here found ELF64 little-endian x86-64 and undefined references to libc-style functions; there is no evidence requiring guessed `-lpng`, `-ljpeg`, or a fabricated `-lsphragis`.
9. Only add a compatibility linker option such as `-no-pie` if a real target linker diagnostic demonstrates the need. Preserve that diagnostic and explanation. Do not suppress arbitrary errors.
10. Default `make` builds all three executables. Provide `clean` that removes only generated student objects and executables; it must never remove the supplied `lib/sphragis.o` or fixture data.
11. Track header dependencies, with explicit prerequisites or compiler-generated `.d` files. Editing a header must rebuild affected objects.
12. Optional targets `test`, `memcheck`, and `package` must be simple, reproducible, and accurately report missing tools/failures.

**Gate:** A clean build can create all three entry points linked against their real dependencies. Stubs are temporary scaffolding only; do not claim functional completion at this gate.

## 5. Step 2: design types, module contracts, and ownership

### 5.1 Required in-memory information

Use simple typed records and dynamic arrays. Counts are data-driven; do not reserve an arbitrary maximum fleet, voyage count, or product list because today's fixtures are small.

| Type concept | Fields that must be preserved | Lifetime/owner |
| --- | --- | --- |
| Endpoint | IP string and port | Embedded in/owned by its configuration or route |
| Food entry | Product string and amount in kg | Odysseus config owns array and strings |
| Odysseus configuration | Name, storage folder, Ithaca endpoint, initial island name/endpoint, gold, food count/list | Odysseus main state |
| Ithaca configuration | Server name, mission folder, listening IP/port | Ithaca main state |
| Voyage | Generated ID, object name, file path, destination, reward | Voyage list owns records/strings |
| Island configuration | Island name, storage folder, listening endpoint, capacity, filtered route list | Island main state |
| Route | Destination name, IP, port | Route list owns records/strings |
| Stock record | Fixed 100-byte name, amount, price | Stock list owns contiguous records |
| Parsed command | Enum kind, extracted argument token(s), validated integer(s) | One terminal iteration; token lifetime explicitly tied to line buffer or duplicated |

Future voyage status may be described in the design, but do not add unused runtime mutation machinery. No generic hash-table/vector framework is necessary. A small typed growable array with count/capacity is enough.

### 5.2 Ownership contract

For each loader, use a documented return status and an output pointer. Recommended convention: on success ownership moves to the caller; on failure the output is left initialized and safe to destroy, with no leaked partial entries.

1. Initialize all pointers to `NULL`, counts/capacities to zero, and descriptors to `-1`.
2. Provide a destroy function for each owning aggregate before implementing its loader.
3. A destructor frees every owned nested string before its array and resets fields. Do not free borrowed tokens or immutable API-returned strings.
4. Only increment a list count after all fields in the new element are successfully owned.
5. Grow arrays using a temporary realloc pointer; verify allocation-size arithmetic before allocating.
6. Store `read`/`write` results in `ssize_t`; do not turn `-1` into a huge `size_t` before checking it.
7. Preserve the original error while cleanup runs so a failed close does not erase the useful diagnostic.
8. Do not make every helper call `exit`. Return failure upward so the process owner performs complete cleanup.
9. Helpers receive state explicitly. No hidden mutable module state.
10. Document ownership of parsed command arguments. A token into a line buffer becomes invalid when that buffer is freed or resized.

### 5.3 Function-size discipline

Keep each function at most 45 physical lines from signature through its closing brace as a conservative audit rule. This counting rule is this guide's interpretation; the style guide does not define its counting mechanics. Aim below the limit rather than squeezing several statements onto one line.

Split by responsibility: acquire a line, validate a number, parse one record, append one record, destroy an aggregate. Avoid splitting merely to hide long logic behind arbitrary `part1`/`part2` helpers.

**Gate:** The design lists each owner, allocation, destructor, status return, and module dependency. It is understandable without reading implementation internals.

## 6. Step 3: implement descriptor I/O and text parsing foundations

### 6.1 Output

Provide a helper such as `writeAll(nFd, pBuffer, nLength)` with a status return.

1. Keep a byte offset and attempt to write remaining bytes.
2. Positive return: advance by exactly that amount.
3. Interrupted write: apply the documented interruption policy and retry only when appropriate.
4. Negative non-interruption error: report failure to caller.
5. A zero-byte write when nonzero data remains must not cause an endless loop; return failure.
6. Do not recursively allocate and report through the same failing helper forever. Keep a fixed-literal best-effort stderr fallback for allocation/output failure.

For dynamic messages, allocate with checked `asprintf`, write the returned number of bytes, then free only on successful allocation. A negative `asprintf` result means the output pointer must not be used or freed based on an assumption that it remained `NULL`.

Use `STDOUT_FILENO` for normal interaction and `STDERR_FILENO` for file/allocation/runtime errors. The exact stream for parser usage diagnostics is not specified; keep a consistent policy and test it. Standard stdin/out/err inherited from the shell do not need to be closed as leaked application-owned descriptors.

### 6.2 File line reader

Adapt the delimiter-reading concept from `Sockets/client.c` without creating a socket. Recommended interface distinguishes `LINE`, `EOF`, `ERROR`, and, if using handler-based interruption, `INTERRUPTED`.

Required behavior:

1. Start with an owned empty buffer of a documented modest capacity or grow from `NULL`.
2. Use `read`, never `fgets` or `getline`.
3. Consume until newline. A one-byte reader is simple for these small configuration files; a buffered reader is also acceptable if it preserves unread bytes.
4. Grow geometrically or in reasonable chunks, not unchecked realloc on every byte.
5. Reserve space for the terminating NUL.
6. At newline return one complete line with the newline removed.
7. On EOF with accumulated bytes, return that final unterminated line once.
8. On EOF with no bytes, return EOF without a fake empty record.
9. A newline with no preceding bytes is an empty line, distinct from EOF.
10. If a carriage return immediately precedes newline, normalize CRLF. Do not trim meaningful spaces inside a folder-path line.
11. Handle interrupted reads without an endless retry after shutdown was requested.
12. On allocation/read failure release temporary buffer ownership appropriately.

Do not copy the sample's `while (c != delimiter)` with no exit when `read` returns zero. Do not write `buffer[n-1]` without proving a positive read containing a newline.

### 6.3 Tokenization

Use the UNIX Appendix A approach: a simple tokenizer, such as `strtok_r` over whitespace, rather than a parser generator. `strtok_r` has explicit parsing state and avoids accidental interference between nested parser calls.

- For commands and tokenized records, accept space/tab separators and count all tokens, including any extras.
- A filename/folder that occupies an entire config line should be retained as a full line, not arbitrarily split.
- The voyage format uses four whitespace-delimited fields. Quoted paths with spaces are not specified and need not be invented.
- Do not lowercase the entire input. Compare command words with `strcasecmp` and preserve file/path/argument spelling.
- If using character-class functions, use an unsigned-char-compatible value.
- If token pointers borrow from a line, duplicate strings that must survive the line's destruction.

### 6.4 Numeric conversion

Prefer `strtol` with end-pointer and range checks; the course samples/books use it. `atoi` cannot distinguish invalid input from a legitimate zero.

Recommended integer helper:

1. Require a nonempty token.
2. Apply the chosen digit/sign grammar explicitly.
3. Set `errno` to zero, call `strtol` in base 10.
4. Require that at least one digit was consumed and the whole token was consumed.
5. Reject `ERANGE` and values outside the destination type's bounds before casting.
6. Apply context bounds: positive command quantity, chosen positive ID policy, valid port range, nonnegative counts/resources as appropriate.

Do not impose a 24-voyage ID upper limit in the command parser. No state lookup is allowed at this phase. Configuration formats may be assumed correct, but I/O/allocation failures still must be handled. Defensive malformed-file checks are recommended and must not reject valid specified configurations.

**Gate:** Text/number helpers handle empty lines, EOF, missing trailing newline, CRLF, very long lines, numeric junk, and allocation/error paths without corruption.

## 7. Step 4: load Odysseus configuration

Source: P PDF pp. 11-12.

Expected order:

```text
Polyphemus
./polyphemus_files
172.16.214.18 8620
Aeaea 172.16.214.20 8621
250
2
Barley 40
DriedFigs 60
```

The IPs above come from the statement's example and are illustrative, not verified deployment assignments. The ports were adapted to the supplied student range. Phase 1 stores these values without contacting them.

Implementation sequence:

1. Check exact CLI argument count before accessing `argv[1]`.
2. Initialize the output configuration.
3. Open the specified file read-only.
4. Read/own the ship name.
5. Read/own the storage-folder line.
6. Parse Ithaca IP and port from the next line.
7. Parse the initial island name, IP, and port. The required initial island is Aeaea; preserve it in the model rather than discarding the name because it is known today.
8. Parse initial gold.
9. Parse the number of distinct food entries.
10. Allocate/grow the food list from this count, with zero count handled safely.
11. Parse exactly that many product/amount pairs and retain each separately.
12. Close the file; return complete owned data.
13. On any error, close the descriptor and destroy partial data in one controlled path.

The configured storage folder is information for later phases. Do not fail Phase 1 because image storage has not yet been created unless instructor guidance adds that requirement. No mkdir/stat workflow is needed merely to load a folder path.

Verification must inspect actual loaded fields, not only the welcome line. For the example, gold is 250 and total food is 100 kg, but Phase 1 `STATUS` must not print a status report.

## 8. Step 5: load Ithaca configuration and voyages

Source: P PDF pp. 12-13, 17.

Configuration:

```text
Ithaca
./objects
172.16.214.18 8620
```

Voyage record grammar:

```text
<OBJECT> <FILE> <DESTINATION> <REWARD>
```

Implementation sequence:

1. Require two CLI arguments after program name.
2. Load name, mission folder, and endpoint from config.
3. Open the separately supplied voyages path read-only.
4. Read records until EOF; do not read a count or ID column.
5. Parse exactly four fields into a temporary voyage.
6. Copy object/path/destination and parse reward safely.
7. Assign the next generated ID only after the record is complete.
8. Append to a dynamically grown voyage array.
9. Dispose of the temporary line each iteration.
10. Handle empty input as zero records unless an instructor requires otherwise; never invent 24 records.
11. Close input; retain the complete list for the process lifetime.
12. Print the loaded count using the required pattern and block until CTRL+C.

For the actual supplied data, the first record is `AeolusBagOfWinds objects/aeolus_bag_of_winds.png Scheria 1150`. Under the recommended ID policy, its ID is 1. `OdysseusBow` is record 3, not record 1 as in a separate illustrative statement excerpt. The last record is `AtlasStarAstrolabe objects/atlas_star_astrolabe.png Thrinacia 900`.

Required sample-style startup/shutdown:

```text
Ithaca initialized. 24 voyages loaded.
Waiting for Odysseus...
Ithaca closes the harbor.
```

The last line appears on shutdown, not immediately after startup. “Waiting for Odysseus” is an initialization message in Phase 1; it does not authorize opening a listening socket.

Store voyage image paths now; image existence/transfer/checksum checks are not a Phase 1 loader requirement. In copied run fixtures, use a lowercase `objects/` folder so the supplied relative paths work later on Linux. Preserve original supplied files in `Phase1/` unchanged.

**Gate:** Exactly 24 supplied records are loaded correctly, IDs are unique and positive under the chosen policy, and changing record count changes the result.

## 9. Step 6: load Island configuration and filter routes correctly

Source: P PDF pp. 13-14, 18; actual L header.

Configuration grammar:

```text
<ISLAND_NAME>
<STORAGE_FOLDER>
<IP> <PORT>
<PORT_CAPACITY>
--- ROUTES ---
<DESTINATION_NAME> <IP> <PORT>
... until EOF
```

The marker is one exact section marker after line normalization. Do not interpret it as a route. There is no route-count line. Every route carries its own endpoint, even when two islands share a host.

### 9.1 Raw configuration loading

1. Require config and stock arguments.
2. Load/own island name, folder, endpoint, and capacity.
3. Recognize the routes marker.
4. Parse each route as a destination name, IP, and port.
5. Keep raw routes explicitly unvalidated; do not publish them as the island's valid route set.
6. Preserve input order.
7. Define a deterministic duplicate policy for defensive parsing, preferably reject duplicate destination records rather than silently losing endpoint identity. Valid specified files are the required case.

### 9.2 Exact library contract

The provided type is:

```c
typedef struct {
    char *name;
    char **known_islands;
    int known_island_count;
} SPHRAGIS_Island;
```

Required operation:

```c
int SPHRAGIS_filter_island_configuration(SPHRAGIS_Island *island);
```

The header states:

- Valid island names are Aeaea, Ogygia, Scheria, Thrinacia, Aeolia, and Ismarus, compared case-insensitively.
- Destination names supplied to the filter must be separately heap allocated.
- The function frees rejected names, preserves the order of survivors, sets unused pointer slots to NULL, and updates the count.
- Caller owns surviving names and the pointer array.
- The return value is the new count; negative error codes are `SPHRAGIS_ERROR_INVALID_ISLAND` or `SPHRAGIS_ERROR_INVALID_CONNECTION`.

Do not treat `result != SPHRAGIS_OK` as failure: a successful return of 2 means two routes survived.

### 9.3 Recommended ownership-safe adapter

Use an independent temporary array of name copies so library mutation cannot leave dangling pointers inside full route records.

1. Keep the original raw route list and all its endpoint/name allocations owned by the loader.
2. Allocate a temporary `char **` array sized to the raw route count; initialize every slot to NULL.
3. Duplicate each destination name into its own allocation in this array.
4. Construct a local `SPHRAGIS_Island`: name borrows the loaded island-name string; names point to the temporary copies; count is the checked raw count converted to int.
5. Call the real filter once the temporary structure is valid.
6. On a negative result, do not accept any routes. Release temporary live pointers according to the documented contract and clean original data. The header does not separately document partial mutation on error; use valid constructed inputs and test actual error behavior on the target before assuming stronger guarantees.
7. On success, iterate the surviving temporary names in order.
8. Match each survivor case-insensitively to its original raw route record.
9. Build a fresh valid route list, copying or explicitly moving the matched name/IP/port together. Copying is simpler for ownership review; release the original raw list only after the new list is complete.
10. If an expected survivor has no original match, treat this as an internal consistency error; never fabricate an endpoint.
11. Free the surviving temporary name copies and the temporary pointer array. Do not free library-rejected names a second time.
12. Free the raw route list, leaving only the valid list in the committed Island configuration.
13. Report the filtered count, not the raw number of lines.

For zero routes, supply NULL names and count zero as appropriate to the API; still invoke mandatory validation of the island configuration. Reject unsupported island names cleanly.

Never hardcode which edges are valid, scrape the binary for topology, or skip the library because the example happens to filter to two routes. The point of the library is to decide valid connections. The report can explain the adapter without revealing or inventing a global route graph.

### 9.4 Filtering tests

- Mix accepted and rejected configured destinations using a disposable test fixture and the actual library as the validation oracle.
- Give each candidate a distinct IP/port so a mistaken post-filter index mapping becomes visible.
- Verify every retained endpoint still belongs to the same destination name.
- Check zero retained routes and multiple retained routes if the library permits those cases for selected input.
- Test unknown island and invalid-array/count handling separately in a small adapter test where appropriate.
- Run under Valgrind to catch double frees of rejected names and leaks of surviving names.
- Do not replace this with a mock filter in the submitted executable.

**Gate:** The stored valid route list is exactly the real library's survivor set with correct endpoints and ownership.

## 10. Step 7: load binary stock

Source: P PDF p. 15 and supplied binary fixtures.

The disk record has these fields in order:

```text
char name[100]
int amount
int price
```

Hungarian field names may differ in authored types without changing layout, for example `sName`, `nAmount`, `nPrice`. Do not alter `sphragis.h` fields. A stock record must not contain a `char *` in place of the on-disk name array.

Observed fixture facts:

| Island file | Bytes | Records | First record | Last record |
| --- | ---: | ---: | --- | --- |
| `Aeaea.db` | 864 | 8 | Barley, 140 kg, 7 gold/kg | EnchantedHerbs, 60 kg, 22 gold/kg |
| `Aeolia.db` | 864 | 8 | Barley, 160 kg, 8 gold/kg | PickledCapers, 140 kg, 9 gold/kg |
| `Ismarus.db` | 864 | 8 | Barley, 260 kg, 3 gold/kg | IsmarianWine, 210 kg, 16 gold/kg |
| `Ogygia.db` | 864 | 8 | Barley, 210 kg, 5 gold/kg | CitrusFruit, 180 kg, 7 gold/kg |
| `Scheria.db` | 864 | 8 | Barley, 180 kg, 6 gold/kg | FineWine, 130 kg, 19 gold/kg |
| `Thrinacia.db` | 864 | 8 | Barley, 300 kg, 4 gold/kg | Chickpeas, 260 kg, 5 gold/kg |

Each was decoded successfully here as 100 bytes followed by two little-endian signed 32-bit integers, totaling 108 bytes. That is an observed fixture layout, not a claim that native C structs are portable across every platform.

Implementation sequence:

1. Define the compatible fixed record type or a clearly documented field-decoding routine.
2. If reading a native struct, verify `sizeof(int) == 4`, expected field offsets, and `sizeof(record) == 108` on Montserrat. Do not add packing pragmas blindly.
3. Open the stock path read-only.
4. Fill one complete record with repeated `read` calls if necessary.
5. EOF before any byte of the next record means clean end of file.
6. EOF after only part of a record means truncated input: report a controlled error; never count it as a full product.
7. Do not use `strlen` to decide the length of binary input.
8. Before treating the name field as a C string, check for termination within its 100 bytes or copy the field into a separately terminated display buffer. Do not write at index 100 of the 100-byte array.
9. Append the complete product to a dynamic list.
10. Close the file and retain the products in memory.
11. Do not modify the database in Phase 1. Verify original data checksums remain unchanged after runs.

No `stat/fstat`, `fseek/ftell`, stdio binary APIs, database engine, or preprocessing conversion is needed. Read until EOF.

Example island output, with counts derived from data:

```text
Island Aeaea initialized.
Port capacity: 2 ships.
<filtered_count> sea routes loaded.
8 products available.
```

After CTRL+C:

```text
Aeaea closes its port.
```

Do not hardcode the example's two routes; the actual filter and supplied configuration determine that number.

## 11. Step 8: implement the command parser

Source: P PDF pp. 15-18 and T PDF p. 2.

### 11.1 Separate parsing from output and future execution

Recommended interfaces conceptually:

- `parseCommand(line, parsedCommand)` returns success, unknown command, or usage error.
- A usage selector returns the relevant usage text.
- Terminal code prints the result.
- No command handler changes ship state, opens a socket, reads a market, renders a map, or checks a voyage list.

A `tParsedCommand` should retain kind plus extracted product/island/ID/quantity in a clearly owned or borrowed form. Avoid validators that discard arguments; extraction is explicitly part of this phase.

### 11.2 Command table and recommended usage strings

| Form | Tokens | Validate | Usage |
| --- | ---: | --- | --- |
| `CONNECT ITHACA` | 2 | Second word exactly ITHACA, case-insensitive | `Usage: CONNECT ITHACA` |
| `LIST VOYAGES` | 2 | Second word VOYAGES | `Usage: LIST VOYAGES` |
| `ACCEPT <id>` | 2 | Entire numeric token, positive-ID policy | `Usage: ACCEPT <voyage_id>` |
| `SAIL <island>` | 2 | Nonempty island token | `Usage: SAIL <island>` |
| `MAP` | 1 | No trailing token | `Usage: MAP` |
| `LIST MARKET` | 2 | Second word MARKET | `Usage: LIST MARKET` |
| `BUY <product> <amount>` | 3 | Nonempty product; positive integer; map-quantity policy | `Usage: BUY <product> <amount>` |
| `SELL <product> <amount>` | 3 | Nonempty product; positive integer | `Usage: SELL <product> <amount>` |
| `STATUS` | 1 | No trailing token | `Usage: STATUS` |
| `DELIVER` | 1 | No trailing token | `Usage: DELIVER` |
| `CLAIM` | 1 | No trailing token | `Usage: CLAIM` |

The supplied tests explicitly fix the ACCEPT, BUY, and MAP usage examples. Other exact placeholder spellings above are recommended for consistency, not additional official golden strings.

For missing/invalid LIST subcommands, recommended output is two lines showing `Usage: LIST VOYAGES` and `Usage: LIST MARKET`. Once a valid LIST subcommand is present but extra tokens exist, show its specific usage. Do not accept prefix matches such as `MAPS` or `ACCEPTED`.

### 11.3 Parsing algorithm

1. Receive one complete line without the newline.
2. Tokenize with explicit lifetime management.
3. If no tokens, apply documented blank-line policy.
4. Compare the first token to recognized verbs without case.
5. Unknown first token: return unknown.
6. Known verb: enforce its exact arity and any required second command word.
7. Convert numeric fields safely and store the validated value.
8. Store argument tokens or owned copies under the parser's documented lifetime.
9. Return success only after every token is accounted for.
10. Print `Command OK\n`, `Unknown command\n`, or the selected usage, then release iteration-owned resources.

Do not enforce “connected to Ithaca,” “docked,” “known island,” “enough gold,” “inventory contains product,” “voyage exists,” or “delivery already complete.” Those are future semantics. For example, `SAIL Atlantis` may be syntactically valid under the token policy even though Atlantis is not a supported deployment island. State this policy clearly; do not silently mix semantic rules into the parser.

### 11.4 Terminal behavior

1. Load Odysseus configuration before showing readiness.
2. Print `Odysseus <name> is ready to sail.\n`.
3. Print `$ ` as the prompt with descriptor output; no stdio flush is needed.
4. Block waiting for input or shutdown.
5. Accumulate arbitrary read chunks into complete lines. A pasted block or redirected file can contain multiple commands per read.
6. Process each line separately and preserve any unfinished suffix for the next read.
7. On EOF, process a final nonempty unterminated command once, then clean up and exit. Do not loop forever on EOF.
8. On CTRL+C with a partial line, discard that partial command and cleanly shut down.
9. Keep output concise. Do not add debug token dumps, unsolicited help banners, extra successes, or stars messages.

**Gate:** All official cases and additional grammar cases pass without stateful behavior.

## 12. Step 9: implement lifecycle and clean interruption

Source: P PDF pp. 17-19; Signals examples; UNIX chapters 3, 4, and 12.

### 12.1 Design constraints

- A single CTRL+C must end every initialized process and free its significant dynamic memory.
- Ithaca/Island must remain alive without reading terminal commands or spinning.
- A process waiting for keyboard input must not require Enter after CTRL+C.
- Handler code must not free or format heap data.
- Do not rely on `while (!flag) pause()` as a race-free design: the signal can arrive after the check and before pause, causing indefinite sleep. The course UNIX book explicitly explains this race in section 12.4.
- A handler plus `read` also needs a plan for the interval between checking a flag and blocking. Merely disabling `SA_RESTART` fixes interrupted reads, not every pre-read timing window.

### 12.2 Recommended course-backed implementation: synchronous signal descriptor

This is a **design recommendation**, not an instructor-mandated API. It combines the UNIX book's `signalfd` (section 12.6) with the course's readiness-wait concept (`Select/poll.c`). It is single-threaded and creates no socket or IPC worker. It avoids global state and unsafe cleanup in handlers.

1. At startup, build a signal set containing SIGINT.
2. Block SIGINT with `sigprocmask`, saving the old mask. Check errors.
3. Create a `signalfd` for that set, using close-on-exec if appropriate. Check failure and preserve cleanup behavior.
4. Hold its descriptor in explicit process state. Do not install a conflicting handler for the same blocked signal.
5. Initialize/load configuration and data. Pending SIGINT can be consumed at safe boundaries; do not ignore it indefinitely while doing unbounded work.
6. Ithaca/Island wait for the signal descriptor and consume its signal record with `read`; then break to normal cleanup.
7. Odysseus blocks in `poll` with stdin plus the signal descriptor. Use a blocking timeout, not a zero-timeout spin or repeated status messages.
8. If SIGINT is ready, consume it and choose shutdown before processing further commands.
9. If stdin is ready, perform `read` into a bounded chunk and accumulate lines. Do not enter a separate blocking read-until-newline loop that ignores the signal descriptor after a partial read.
10. Handle `POLLHUP` by draining remaining input and reaching EOF; handle `POLLERR/POLLNVAL` as controlled errors. Handle EINTR according to policy.
11. Close the signal descriptor and all owned file descriptors; destroy owned aggregates in reverse acquisition order.
12. Keep shutdown behavior defined if additional SIGINT signals arrive during cleanup. Do not restore the default SIGINT behavior before significant owned resources have been released. Restore an old mask only with a deliberate pending-signal policy, or finish the process after cleanup without exposing a new unsafe interval.

If local teaching expectations prefer handlers, use `sigaction`, a `volatile sig_atomic_t` flag, and mask-aware waiting (`sigsuspend` for idle servers, an equivalent race-free terminal strategy). Document the complete strategy and test its edge cases; do not copy allocating sample handlers. If a handler flag is unavoidable, keep its definition in the corresponding main process, not a shared module/header definition.

### 12.3 Partial initialization cleanup

For each process, list acquisitions in order: signal resources, config descriptor, config strings/arrays, data descriptor, data records, terminal buffer. Test failure immediately after each meaningful stage where practical.

- A missing stock file after successful route filtering must still release routes and all config fields.
- A malformed voyage after many valid entries must release all earlier entries and the partially parsed current entry.
- A rejected route name must not be freed twice.
- A failed terminal formatting allocation must not strand the loaded configuration.
- Owned descriptors are closed once, even if their numerical value is zero because stdin was originally closed.

Error exits should use a documented nonzero status. EOF and normal controlled shutdown can use a documented success/interruption convention; the statement does not prescribe an exact signal exit code. Tests should check the chosen convention without pretending it is an official requirement.

**Gate:** Single CTRL+C at idle, partial input, and after many commands reliably exits; no busy wait, invalid access, or owned-resource leak appears.

## 13. Step 10: verify compliance with the coding style

Source: S sections 2-9.

### 13.1 File structure and includes

- File header identifies file, purpose, actual author(s), and last modification date.
- Entry-point purpose documents CLI arguments.
- Own `.c` files include their own project headers; put required system headers into associated `.h` files as the guide requests.
- Headers have include guards, system includes before project includes, constants/types, and prototypes in the guide's order.
- Prefer nonreserved include-guard names such as `NOSTOS_IO_H`. The guide illustrates double-underscore names, but its substantive requirement is preventing multiple inclusion; preserving safe C identifiers is a documented convention choice.
- Do not define mutable application globals in a common header. `extern` declarations do not allocate a second object.
- Preserve required vendor header/object unmodified and outside student-style edits.

### 13.2 Names and local declarations

| Category | Guide pattern |
| --- | --- |
| int | `nAmount`, `nFd`, `nPort` |
| long | `lParsedValue` |
| short | `shValue` |
| unsigned char / unsigned int | `byFlags`, `wFlags` |
| fixed C string | `sName[100]` |
| pointer to string | `psName` |
| other pointer | `pnValues`, `pstConfig` |
| array | `anPorts` or a type-appropriate array prefix |
| struct / typedef / enum | `st...`, `t...`, `e...` |
| unavoidable global | `g` before the usual type prefix |
| function | `loadIslandConfig`, `parseCommand`, `destroyVoyages` |
| constant | `STOCK_NAME_SIZE`, `COMMAND_OK` |

The guide does not assign every POSIX typedef, such as `size_t` and `ssize_t`, a special prefix. Choose and document consistent descriptive names without incorrectly changing the type to int to satisfy naming. External struct member names are not under your control.

Declare locals at the beginning of their function, initialize them, and avoid multiple declarations/assignments on one line. Use the smallest sensible scope. Loop-only `i`/`j` are explicitly permitted, but descriptive counters are often clearer.

### 13.3 Control flow and comments

- `if (NULL == psName) {` illustrates constant-first comparisons. Preserve mathematical meaning when reversing relational comparisons.
- Same-line opening braces; four-space indentation; operators and comma-separated arguments spaced consistently.
- No chained `a = b = value` assignments.
- Use ordinary `if` instead of unnecessary ternaries.
- Every switch has a default and explicit branch termination; follow the guide's break rule rather than accidental fall-through.
- Avoid goto-based cleanup; small destroy helpers and explicit status propagation keep cleanup readable.
- Comment above code, explaining purpose/ownership or an unusual constraint rather than restating an obvious assignment.
- Every function, including private helpers, has the specified descriptive header.
- Count function lengths manually or with an appropriate parser; a naive brace-count regex is not a reliable final audit.

### 13.4 Forbidden API review

Scan **authored runtime source**, not `All Materials`, original PDFs, or the vendor object. Review hits for actual calls, not substrings inside documentation.

At minimum inspect for stdio I/O, `system`, `popen`, stat-family APIs, socket APIs, fork/thread/IPC APIs, and hidden wrapper calls to prohibited operations. `asprintf` and `snprintf` are allowed; do not flag them merely because they contain `printf` in their names.

The supplied `.o` imports stdio functions. A final-binary symbol scan alone cannot attribute those calls to the student. Keep a source audit and, where useful, inspect undefined symbols on student-produced object files separately. Do not modify the vendor object to make a superficial grep pass.

## 14. Step 11: functional and robustness test plan

The official test sheet is short and explicitly nonexhaustive. Keep a small reproducible harness that launches the actual executables; Python/shell may orchestrate tests but must not become runtime dependencies or preprocess data for the C implementation. Course I/O restrictions apply to authored C runtime code; shell testing commands such as `printf` are not calls to C `printf` in that code.

Record exact input, expected result, actual result, exit status, and test environment. Match required message text while accounting for prompts/welcome lines separately. Do not count a prompt as part of a response string by accident.

### 14.1 Official positive command cases

Every line below yields `Command OK`:

```text
CONNECT ITHACA
LIST VOYAGES
ACCEPT 2
SAIL Aeaea
MAP
LIST MARKET
BUY DriedFish 3
SELL Barley 10
STATUS
DELIVER
CLAIM
cOnNeCt iThAcA
```

Run them immediately after startup without establishing any real connection. Otherwise a semantic restriction bug could be hidden by test setup.

### 14.2 Official negative cases

| Input | Exact required output from supplied tests |
| --- | --- |
| `ACCEPT` | `Usage: ACCEPT <voyage_id>` |
| `BUY DriedFish` | `Usage: BUY <product> <amount>` |
| `MAP now` | `Usage: MAP` |
| `something else` | `Unknown command` |

### 14.3 Expanded command tests

| Class | Examples | Expected under this guide's documented choices |
| --- | --- | --- |
| Case | `list market`, `sTaTuS`, `bUy DriedFish 3`, `buy map 1` | Command OK |
| Whitespace | leading/trailing spaces, repeated spaces, tabs | Same parse result |
| Missing fixed word | `CONNECT`, `LIST` | Usage |
| Wrong fixed word | `CONNECT Aeaea`, `LIST cargo` | Usage; LIST shows supported alternatives |
| Missing argument | `SAIL`, `BUY`, `SELL Barley` | Usage |
| Extra argument | `CONNECT ITHACA now`, `LIST VOYAGES 2`, `ACCEPT 2 3`, `SAIL Aeaea now`, `STATUS now`, `DELIVER x`, `CLAIM x`, `BUY Barley 2 x` | Usage |
| Prefix impostor | `MAPS`, `ACCEPTED 2`, `BUYER X 1` | Unknown command |
| Numeric junk | `ACCEPT abc`, `ACCEPT 2x`, `BUY Barley 3kg`, `SELL Barley 1.5` | Usage |
| Nonpositive quantity | `BUY Barley 0`, `SELL Barley -2` | Usage |
| ID lexical policy | `ACCEPT 0`, `ACCEPT -1`, `ACCEPT +2` | Usage under recommended positive/digit-only policy |
| Numeric representation | `ACCEPT 0002`, `BUY Barley 0003` | Command OK |
| Overflow | integer string above LONG_MAX or INT_MAX | Usage, no wraparound |
| State independence | `ACCEPT 999`, `SAIL Atlantis`, `BUY UnknownProduct 3`, `DELIVER`, `CLAIM` | Command OK under syntax-only policy |
| Map special form | `BUY MAP 1`, `BUY MAP 2`, `SELL MAP 1` | OK, usage, OK respectively under recorded interpretations |
| Empty input | blank line, whitespace-only line | Reprompt only |
| Long input | long unknown verb, long product token, many extra tokens | Correct classification or documented controlled limit, no overflow/hang |
| Stream boundaries | many commands in one write; one command in several chunks | Exactly one result per complete command |
| EOF | empty stdin; final command without newline | Clean exit; final command processed once |

Do not silently truncate an overlong token into a recognized valid command. Dynamic input is preferable to arbitrary small limits; if a defensive limit is used, it must have a documented rationale and explicit rejection/drain behavior.

### 14.4 Loader tests

1. Compare every field of each valid config against input using a small loader harness or debugger evidence. Startup messages alone do not prove fields were stored.
2. Run each CLI with missing and extra arguments. Expect usage and controlled nonzero exit.
3. Try nonexistent config/voyage/stock paths separately.
4. Try a file that exists but is unreadable under the test user; do not run a permission test as root and call it representative.
5. Repeat text fixtures with CRLF and without a final newline.
6. Use zero and multiple food entries; verify count/list handling.
7. Use a longer voyage file to demonstrate dynamic storage; do not hardcode fixture counts.
8. Test an empty voyage file and empty stock file under the documented zero-record policy.
9. Try an incomplete final binary stock record in a disposable copy. Expect a controlled error, not eight plus a phantom product.
10. Test each of the six real stock files and check names, amounts, prices, and count.
11. Test route filtering and endpoint preservation as described in section 9.4.
12. Confirm opening failures late in initialization free everything allocated earlier.
13. Confirm repeated runs do not mutate source configs, voyages, stock files, or PNGs.

### 14.5 Lifecycle tests

Use separate terminal sessions or a PTY-based harness for real CTRL+C behavior. A literal byte `0x03` written into a regular pipe is not the same as the terminal generating SIGINT.

- Ithaca: initialize, wait several seconds, verify still alive and low idle CPU, send SIGINT, verify cleanup.
- Island: same for each configured island, using the corresponding stock file.
- Odysseus: SIGINT at an empty prompt, after partial input, after many valid/invalid commands, and around input-read boundaries.
- EOF with no command and EOF after an unterminated command.
- Output redirected to a file: messages appear in order and no stdio buffering dependency appears.
- Closed/failed input and output conditions where practical; no infinite diagnostic loop.
- Repeated starts/stops: stable ownership and no growing leaks.
- Only one Ithaca is active in the default deployment test; record A03 rather than falsely claiming runtime singleton enforcement was proved.

### 14.6 Memory and descriptor verification

Build with debugging information. For each executable, use the course Valgrind guidance, for example:

```sh
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes ./odysseus configs/odysseus.dat
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes ./ithaca configs/ithaca.dat data/voyages.dat
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes ./island configs/aeaea.dat data/stocks/Aeaea.db
```

Terminate interactive processes through SIGINT so the intended cleanup path is exercised. Also run representative missing-file, truncated-stock, route-filter, and command-stress paths.

Expected: no invalid reads/writes, use-after-free, double frees, uninitialized output, or lost student allocations. Investigate still-reachable blocks and distinguish application state from runtime/vendor allocations using stack traces. Do not suppress unknown failures or claim a clean result without examining logs. Descriptor reports may list inherited stdin/stdout/stderr; application-opened files and signal descriptors must not remain open.

Allocation-failure testing is recommended where a controlled test allocator or fault-injection facility is available. Keep it out of the release build and label coverage honestly. At minimum inspect every allocation error path; do not claim exhaustive failure injection if none was performed.

### 14.7 Build/style gate

- `make clean` followed by `make` succeeds with no warnings.
- A second `make` performs no needless rebuild; touching a header rebuilds its dependents.
- Each executable has its own main; no accidental linking of multiple mains.
- No generated object is required except the intentionally supplied vendor object.
- No prohibited APIs or future-phase mechanisms in authored runtime code.
- All style checks in section 13 pass.
- No compiler-warning suppressions used to hide defects.
- Actual Sphragis is linked and invoked.

## 15. Step 12: prepare report, README, and student understanding

### 15.1 Report requirements

P PDF p. 39 requires the report in partial deliveries; the final PDF/report contents are detailed on p. 40. For Phase 1, create an evolving report that accurately describes completed Phase 1 work and separates future plans.

Include:

1. Cover: project, academic year, actual student(s), group, phase, date.
2. Table of contents.
3. Design: independent processes, modules and dependencies, data structures and justification, resources/APIs used and why.
4. Block diagrams and flowcharts. A Phase 1 diagram must not suggest live network connections; any future network diagram must be labeled future design.
5. Observed problems and actual solutions, including Sphragis ownership, file layout, EOF, partial reads, and shutdown if encountered.
6. Time spent per student in research, design, implementation, testing, and documentation. Use actual records; leave unknown historical time pending rather than inventing hours.
7. Conclusions and proposed improvements.
8. Optional explanation of the project/process names and theme, supported by the statement.
9. Bibliography in a consistent format, citing course notes, samples, statement, and any actually used additional references.

The final report must have numbered pages and be PDF. A numbered PDF progress report is a useful Phase 1 choice as well, but distinguish that recommendation from the statement's explicit final-PDF requirement. Review any exported report for legible diagrams, tables, and page numbers.

### 15.2 README

Document prerequisites, where to run `make`, three CLI forms, working-directory/path convention, how to select an island configuration and matching stock, expected startup messages, CTRL+C/EOF behavior, and how to run tests/Valgrind. Explain that commands are intentionally syntax-only in Phase 1.

Never embed a student's password or private SFTP configuration. Distinguish university hostnames from unverified sample IPs. The port list gives Salah's range as 8620-8624; Phase 1 does not open those ports. For later phases, seven server identities across multiple machines do not require inventing seven new ports on one machine.

### 15.3 Interview preparation

The supplied rubric assigns 5 points to execution, 2 to documentation, and 3 to interview. A working generated program alone does not satisfy the interview criterion. Include a short explanation of each module and require the student to be able to answer:

- Why use read/write instead of printf/fgets?
- Why does read not produce a C string automatically?
- How are EOF, a blank line, an error, and a partial record different?
- Who owns each allocation and how is partial initialization destroyed?
- Why does realloc need a temporary pointer?
- What does Sphragis free, and how are route IP/port pairs preserved?
- Why does `ACCEPT 999` succeed syntactically now?
- Why are stock records fixed-size binary and why does ABI matter?
- How does one CTRL+C wake a waiting process without unsafe handler cleanup?
- Why is an idle blocked process different from a busy loop?
- What would Phase 2 add without rewriting the parser/loaders?
- What did the actual tests prove, and what remains unverified?

## 16. Step 13: package and test the submission

Source: P PDF pp. 20, 38-40.

1. Resolve actual group number and author details. `G12_F1.tar` is the statement's example, not this user's established group.
2. Ensure the package contains authored source/headers, Makefile, real Sphragis header/object, required data/configuration, README/run guidance, and progress report.
3. Include supplied object images if they are referenced by your documented runnable fixture set, while keeping clear that Phase 1 only stores their paths.
4. Exclude credentials, original course textbooks, extraction scratch files, unrelated generated binaries, caches, and stale build outputs.
5. Choose a consistent archive layout: either a top-level `nostos/` directory or contents directly at root, with the README explaining the build location. Nested module directories must be included; the statement's flat wildcard command is only an example.
6. Create an actual tar archive named `G<actual_group>_F1.tar`; a renamed zip is unacceptable.
7. Run `tar tf` to inspect the archive listing.
8. Extract into a new empty test directory.
9. Run `make` inside that extracted project, without access to the original materials through absolute paths.
10. Run all three CLIs against the archived fixtures and perform CTRL+C checks.
11. Perform this verification on Montserrat before calling it submission-ready.
12. Keep submission/upload as a separate user action unless explicitly requested.

The supplied schedule lists Phase 1 on 04/10/2026, Phase 2 on 01/11/2026, full-score final opportunities on 22/11/2026 and 06/12/2026, and a later out-of-7 opportunity on 10/01/2027. These are local-document dates, not a live deadline verification. Do not infer a new deadline from old filenames.

## 17. Future architecture notes: read now, implement later

These notes keep Phase 1 models useful without adding premature code.

### Phase 2

- Separate TCP processes on different hosts; Ithaca and Aeaea must be on different machines. Use configured/discovered endpoints, not localhost assumptions or cross-host shared files.
- Multiple Odysseus clients; each voyage assigned to at most one client. Protect entire check-and-update operations.
- Island ports have limited capacity and explicit FIFO waiting. A semaphore alone does not establish the required application FIFO policy.
- Waiting ships consume 7 kg for each full 5 seconds offshore, but not while sailing or docked. Wait on time and network events without spinning.
- Purchases/sales maintain consistent stock and update binary stock files.
- Maps cost 50, are not stock records, do not run out, and can be purchased again. `BUY MAP 1` adds filtered bidirectional routes and their endpoints; MAP only displays known information.
- An island knowing a neighbor does not mean Ithaca knows the route, and a ship knowing an island does not mean it can sail there directly from its current island.
- Phase 2 reaches the destination but does not transfer objects or complete receipts.
- Read the precise reconnection/failure rule: after a lost ship restarts, the previous mission stays assigned until that Odysseus executes LIST VOYAGES again, at which point Ithaca marks it failed before returning availability. Do not casually invent “immediately make every mission available on disconnect.”

### Phase 3

- Transfer object images, deliver to destination, receive the same image stamped with the island glyph, return directly to Ithaca, and claim reward after mission/glyph validation.
- Use real Sphragis signing/checking operations with the header's ownership rules. `SPHRAGIS_identify_glyph` returns an immutable string that must not be freed.
- Execute the OS `md5sum` program through course fork/exec/pipe mechanisms; no custom MD5 and no system/popen.
- Preserve Odysseus state and Island stock between runs; consult precise death/reset/persistence interactions before coding them.
- No extra secondary-memory temporary transfer state beyond the system's own files, per P p. 37.

### Protocol facts and an unresolved future conflict

Annex II specifies exactly 256 bytes per serialized frame: 1 type byte, 1 flags byte, 1 numeric data-length byte, 253 data bytes. Unused data bytes are zero. Requests/responses retain the defined type and flags; binary payloads must be handled by length rather than C-string functions. Do not write a native struct as the wire format.

The statement PDF p. 42 explicitly describes a single 256-byte read/write, while the UNIX course explains that TCP is a byte stream and may produce short reads/writes. This is a real future design clarification: preserve the required serialized frame layout and discuss short-I/O recovery with instructors. Do not misrepresent TCP as guaranteeing one read per write or resolve this conflict by changing the protocol. It does not affect Phase 1's file/terminal read loops.

All transfers follow header 0x20, READY 0x22, binary chunks 0x21, final ACK 0x23. These protocol details are context only; do not create a Phase 1 networking module just to hold unused constants.

## 18. Audit handoff that Claude must leave behind

Create `nostos/docs/audit-handoff.md` with:

1. Implemented phase and explicit exclusions.
2. Requirement mapping F01-F20 to real source files/functions and tests.
3. Module map and allocation/descriptor ownership table.
4. Exact compiler/linker commands, platform, and source/library hashes where useful.
5. Functional test commands and observed outputs.
6. Valgrind summaries and paths to complete logs, including signal and failure paths.
7. Style/forbidden-API audit results scoped to authored code.
8. Sphragis integration evidence and endpoint-preservation test.
9. Archive listing and fresh-extraction build/run evidence.
10. Assumptions A01-A16 with instructor-confirmed, selected-default, or unresolved status.
11. Known issues, missing remote access, unrun checks, and any unsupported claims removed from the README/report.
12. A short student walkthrough for the interview.

### Reviewer checklist

- [ ] All three programs exist, build, and use the exact CLI forms.
- [ ] No loader silently drops required data.
- [ ] No hardcoded fixture counts/results.
- [ ] Correct real binary-stock layout and EOF handling.
- [ ] Actual Sphragis filtering, correct success interpretation, ownership and endpoint matching.
- [ ] Every command recognized, case handled, arguments extracted, numeric conversion checked.
- [ ] Success/unknown/usage messages correct; extra tokens rejected.
- [ ] No premature semantic checks or Phase 2/3 implementation.
- [ ] Safe CTRL+C and EOF lifecycle without busy waits or signal-handler heap work.
- [ ] Error exits release partial state and owned descriptors.
- [ ] No warnings, prohibited API shortcuts, or accidental vendor deletion in clean.
- [ ] Style guide applied, including Hungarian names, headers, includes, comments, and short functions.
- [ ] Report, README, bibliography, and interview explanations match actual code.
- [ ] Genuine self-contained tar verified on Montserrat, or remote verification explicitly pending.
- [ ] Human design validation and ambiguous requirements honestly recorded.

## 19. Prompt to start Claude Code

Paste this after making the whole folder available to Claude:

> Read CLAUDE.md and PHASE1_IMPLEMENTATION_GUIDE.md completely, then inspect the cited originals, particularly Phase1/Style Guide.pdf, the Nostos Phase 1 requirements, sphragis.h, and All Materials/Codeing samples. Implement Phase 1 only in nostos/, following the ordered guide and course methodology. Preserve the original materials. Start with module/ownership design and record unresolved assumptions; never fabricate instructor validation. Build all three required C executables, integrate the real Sphragis library, implement loaders and syntax-only command parsing, and verify cleanup, errors, style, and memory. Finish with reproducible tests, a progress report, README, and docs/audit-handoff.md for an independent audit. Distinguish tests actually run from pending Montserrat checks. Do not implement later phases or submit anything to eStudy.

For the later Codex audit, return the complete `nostos/` source/data/docs folder or its verified tar, together with actual compiler/test/Valgrind logs and any instructor clarifications. A screenshot of successful output is not enough to audit ownership, restrictions, or build reproducibility.
