# Nostos Phase 1 — module and ownership design

Status: **prepared for monitor/instructor review, not yet validated** (see `assumptions.md`
A02). This document is the artifact the statement (P p.38, Annex III) asks to be validated
before/while implementing Phase 1; producing it does not itself constitute validation.

## 1. Process/module map

Three independent executables, no forks, no IPC between them in this phase.

```
                +----------------+
 argv --------> |   Odysseus.c   |  only process with a terminal
                +----------------+
                | Odysseus.h     |  CLI usage text (entry-point only)
                | config.c/.h    |  loadOdysseusConfig / destroyOdysseusConfig
                | commands.c/.h  |  parseCommand (pure, no I/O)
                | lifecycle.c/.h |  blockSigint / createSigintFd / waitForSignal
                | text.c/.h      |  tLineBuffer, tokenizer, numeric parsing
                | io.c/.h        |  writeAll / writeString / writeFormatted / safeRead
                +----------------+

                +----------------+
 argv --------> |   Ithaca.c     |  no terminal, blocks until SIGINT
                +----------------+
                | Ithaca.h       |
                | config.c/.h    |  loadIthacaConfig / destroyIthacaConfig
                | voyages.c/.h   |  loadVoyages / destroyVoyageList
                | lifecycle.c/.h |  (shared)
                | text.c/.h, io.c/.h (shared)
                +----------------+

                +----------------+
 argv --------> |   Island.c     |  no terminal, blocks until SIGINT
                +----------------+
                | Island.h       |
                | config.c/.h    |  loadIslandConfig (name/folder/endpoint/capacity)
                |                |  + loadIslandRawRoutes (unfiltered candidates)
                | routes.c/.h    |  filterIslandRoutes -> real SPHRAGIS adapter
                | stock.c/.h     |  loadStockList / destroyStockList
                | lifecycle.c/.h |  (shared)
                | text.c/.h, io.c/.h (shared)
                +----------------+

                lib/sphragis.h, lib/sphragis.o   <- vendor, unmodified, linked only into island
```

Rationale for combining Odysseus/Ithaca/Island configuration loading into one `config.c`:
the three formats share the same line-by-line, whitespace-endpoint grammar and the same
"open, read lines, parse, close, or destroy partial state" control flow; splitting them
into three files would triplicate the shared endpoint/line helpers without adding a
distinct responsibility. `routes.c` is kept separate from `config.c` because it has a
genuinely distinct responsibility (the Sphragis ownership adapter, Section 4 below) with
its own nontrivial invariants. Each loader function individually stays within the 45-line
limit; splitting further would produce artificial `partN` helpers the guide explicitly
warns against.

`Odysseus.h`/`Ithaca.h`/`Island.h` are kept intentionally small (an `@Purpose` comment and,
where useful, a `printUsage()` declaration for that binary's own CLI message) rather than
empty placeholders, since each entry-point file only exposes `main()`.

## 2. Core types and ownership (`include/types.h`, `include/status.h`)

| Type | Fields | Owner / lifetime |
| --- | --- | --- |
| `tEndpoint` | `psIp` (owned), `nPort` | embedded in the struct that owns it |
| `tFoodEntry` | `psProduct` (owned), `nAmountKg` | array element owned by `tOdysseusConfig.pstFoods` |
| `tOdysseusConfig` | name, storage folder, Ithaca endpoint, initial island name + endpoint, gold, food array/count | `Odysseus.c` main-state, one instance, whole-program lifetime |
| `tIthacaConfig` | name, mission folder, listen endpoint | `Ithaca.c` main-state |
| `tVoyage` | generated `nId`, object, file path, destination, reward | element of `tVoyageList.pstVoyages` |
| `tVoyageList` | `pstVoyages`, `nCount`, `nCapacity` | `Ithaca.c` main-state |
| `tRoute` | destination (owned), IP (owned), port | element of `tRouteList.pstRoutes`; used for both the *raw* (unfiltered) and *valid* (post-Sphragis) lists |
| `tRouteList` | `pstRoutes`, `nCount`, `nCapacity` | raw list: local to `Island.c`'s init sequence, destroyed once filtering finishes; valid list: embedded in `tIslandConfig`, whole-program lifetime |
| `tIslandConfig` | name, storage folder, listen endpoint, capacity, `stRoutes` (valid only) | `Island.c` main-state |
| `tStockRecord` | `sName[100]` (raw fixed bytes, **not** guaranteed NUL-terminated), `nAmount`, `nPrice` | element of `tStockList.pstRecords` |
| `tStockList` | `pstRecords`, `nCount`, `nCapacity` | `Island.c` main-state |
| `tParsedCommand` | `eKind`, `psArg1` (owned, may be NULL), `lNumber`, `nHasNumber` | one terminal iteration; destroyed immediately after the result is printed |
| `tUsageMessage` | up to 2 **borrowed** `const char *` literals + count | one terminal iteration; nothing to free (string literals) |

Ownership rules applied uniformly (guide Section 5.2):

1. Every `init*`/loader zeroes/NULLs all pointers and sets counts to 0, descriptors to -1,
   *before* any fallible operation, so a `destroy*` call is always safe even after a
   partial failure.
2. Every owning aggregate has a matching `destroy*` written *before* its loader is
   implemented (see commit history / function ordering inside each `.c` file).
3. A dynamic array's count is only incremented after every field of the new element has
   been successfully allocated; a failure mid-element frees only that element's own
   partial allocations, then propagates failure — the caller's `destroy*` handles the
   array itself.
4. `realloc()` is always called into a temporary pointer; the original array pointer is
   only overwritten after the temporary is checked non-NULL.
5. `read()`/`write()` results are always stored in `ssize_t` and checked before any
   arithmetic or cast to `size_t`.
6. A failed `asprintf`/`vasprintf` leaves its output pointer unspecified by POSIX; call
   sites never read or free that pointer when the return value is negative.

## 3. I/O and text foundations (`src/io.c`, `src/text.c`)

* `io.c` — `writeAll`, `writeString` (literal helper), `writeFormatted` (checked
  `vasprintf` + `writeAll` + `free`, per the course's format→write→free idiom),
  `safeRead` (EINTR-retrying wrapper distinguishing data / EOF / error), `safeOpenReadOnly`.
  `STDOUT_FILENO` is used for the `$ ` prompt and command results; `STDERR_FILENO` is
  used for file/allocation/runtime error diagnostics.
* `text.c` — `tLineBuffer` (owned growable byte buffer used both for reading whole
  configuration/data files line-by-line and for accumulating partial terminal input
  across multiple `read()`s), a `strtok_r`-based tokenizer producing an owned
  `char **`/count pair, and `parseDigitsToLong` (the single numeric-grammar entry point
  used by every numeric field in the project: voyage rewards/gold/food amounts inside
  config loaders, and command quantities/IDs inside the parser).

Only one line-reading abstraction exists (`tLineBuffer`) and is reused by:
  * `config.c` / `voyages.c` (loop: read a chunk with `safeRead`, append, drain complete
    lines, stop at EOF after taking one final unterminated line if non-empty), and
  * `Odysseus.c`'s terminal loop (same abstraction, but chunks arrive interleaved with
    `poll()` readiness instead of a tight loop, and shutdown can interrupt it between
    chunks).

## 4. The Sphragis ownership adapter (`src/routes.c`)

`SPHRAGIS_filter_island_configuration()` **frees rejected names in place** and expects
**separately heap-allocated** destination-name strings it can free safely. The raw route
list already owns its own name/IP allocations (needed regardless, to preserve IP:port for
survivors), so those pointers are never handed to the library directly — a second,
throwaway array of name *copies* is built just for the call:

1. Allocate `char **ppsTempNames[nRawCount]`, all slots NULL.
2. `duplicateString()` each raw route's destination name into its own slot. Any
   duplication failure frees what was copied so far and aborts before calling the library
   (raw list is untouched; caller destroys it normally).
3. Build one local, stack-allocated `SPHRAGIS_Island` borrowing the island's own name and
   pointing `known_islands` at the temporary array.
4. Call the real `SPHRAGIS_filter_island_configuration()` exactly once.
5. Negative return: defensively free any temp slot the library left non-NULL (see
   assumption A19), free the array, propagate the error — no route is kept.
6. Non-negative return `n`: walk the temporary array; every surviving (non-NULL) slot is
   matched case-insensitively back to its original raw route (by destination name) to
   recover the IP/port the library does not track. The **survivor's own string is moved**
   (not re-copied) into the new `tRoute`, and the matched slot is set to NULL so the final
   cleanup pass never double-frees it. An unmatched survivor (should be impossible given
   step 1-2) is treated as an internal consistency error and aborts filtering rather than
   fabricating an endpoint.
7. The temporary array (now all NULL) and the raw route list are freed; only the new,
   independently-owned valid list is kept, with `nCount == n`.

Zero raw routes still calls the library (`known_islands = NULL`, `count = 0`) so an
invalid *island name* is still caught even when the routes section is empty.

## 5. Command parser (`src/commands.c`)

`parseCommand()` is pure: it takes one already-dequeued line (no trailing `\n`), tokenizes
it, classifies the first token against the eleven verbs case-insensitively via exact
`strcasecmp` (never a prefix match, so `MAPS`/`ACCEPTED`/`BUYER` fall through to
`Unknown command`), validates arity/second-word/numeric fields per the table in
`PHASE1_IMPLEMENTATION_GUIDE.md` Section 11.2, and returns one of
`PARSE_OK` / `PARSE_UNKNOWN` / `PARSE_USAGE` / `PARSE_EMPTY`, or `PARSE_ERROR` when one of
its own allocations fails (reported by `Odysseus.c` as a resource error on stderr, never
as `Unknown command`; `PARSE_UNKNOWN` is reserved for an unrecognized verb). It never touches a file
descriptor and never inspects voyage/route/market state — Phase 1 explicitly forbids that
(P p.19). All *printing* (`Command OK\n`, `Unknown command\n`, one or two `Usage:` lines)
happens in `Odysseus.c`'s terminal loop, which is the only place descriptor writes happen
for command results.

## 6. Lifecycle and shutdown (`src/lifecycle.c`)

Race-free design per the UNIX book Section 12.6 (`signalfd`), avoiding the classic
check-then-`pause()` race the same book documents in 12.4:

1. `blockSigint()` — build a `sigset_t` containing only `SIGINT`, `sigprocmask(SIG_BLOCK,
   ...)`, save the old mask. Done before any file is opened, so a CTRL+C during
   initialization is queued, never lost, and never handled mid-allocation.
2. `createSigintFd()` — `signalfd(-1, &set, SFD_CLOEXEC)`. The returned descriptor is
   ordinary application-owned state (closed once during cleanup), not a global.
3. Ithaca/Island: `waitForSignalOnly()` blocks in `poll()` on that one descriptor with an
   infinite timeout — genuinely asleep, not a busy loop — then does one `read()` of the
   `struct signalfd_siginfo` to consume the queued signal before falling through to
   ordinary, non-handler cleanup code (free lists, close descriptors, close the signalfd).
4. Odysseus: the terminal loop `poll()`s **two** descriptors (`STDIN_FILENO` and the
   signalfd) with an infinite timeout. A ready signalfd is drained and shutdown begins
   immediately, even with a partial command line pending (that partial line is discarded,
   per P p.19 "no infinite loops... under any circumstances"). A ready stdin is read in
   one bounded chunk, appended to the persistent `tLineBuffer`, and every complete line it
   now contains is parsed/executed/printed before returning to `poll()`. `POLLHUP` (stdin
   closed, e.g. redirected-from-file EOF) drains any final unterminated line once, then
   shuts down the same way SIGINT would.
5. No handler ever runs: because `SIGINT` stays blocked for the process's entire life
   (never restored), there is no signal-handler context anywhere in this codebase, so the
   "no allocation/free/formatting in a handler" rule is satisfied by construction rather
   than by careful handler discipline.

## 7. Partial-initialization cleanup order

Each entry point acquires resources in a fixed order and tears down in reverse, so a
failure at step *k* only needs to undo steps *1..k-1* (already-initialized destructors are
always safe to call on zeroed/partial state per Section 2, rule 1):

`Odysseus`: block SIGINT → create signalfd → load config → (destroy config) → close
signalfd → restore mask (only after everything else, or not at all before `_exit`).

`Ithaca`: block SIGINT → create signalfd → load config → load voyages → wait for signal →
(destroy voyages) → (destroy config) → close signalfd.

`Island`: block SIGINT → create signalfd → load config → load raw routes → filter routes
(consumes raw, produces valid) → load stock → wait for signal → (destroy stock) →
(destroy config, which owns the valid route list) → close signalfd.

A failure at any loading step releases everything acquired so far in the same order,
using the same `destroy*` functions cleanup uses on the success path — there is exactly
one cleanup path per resource type, reached either from the error branch or from the
post-signal branch.

## 8. What this design deliberately does not include

No sockets, no `fork`/`exec`, no threads, no IPC objects, no persistence, no market/trade
mutation, no real map rendering, no glyph signing/checking, and no state machine tracking
"connected"/"docked"/"has an active voyage" — Phase 1's parser is syntax-only by explicit
statement rule (P p.15-16, 19) and this design does not smuggle any of that in through a
"helpful" validation.
