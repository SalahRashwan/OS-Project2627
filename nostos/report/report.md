# The Nostos System: Progress Report

**Project:** The Nostos System (Operating Systems, La Salle)
**Academic year:** 2026-27
**Phase:** 1, Inithaca
**Student:** Salah Ahmed Salaheldin Adly Rashwan
**Group:** (to be completed)
**Date:** 26 September 2026 (first version 21 September 2026)

This is the progress report for the Phase 1 partial delivery (statement p. 39). It
describes what has been implemented, why, and what is still open. It is not the final
report and does not claim any instructor validation.

## 1. Table of contents

1. Table of contents
2. Design overview
3. Data structures and justification
4. Resources/APIs used and why
5. Problems encountered and solutions
6. Block diagram
7. Time spent
8. Conclusions and proposed improvements
9. Theme note
10. Bibliography

## 2. Design overview

The system has three independent processes: `odysseus`, `ithaca` and `island`. Each
one is built from its own entry-point file (`Odysseus.c`, `Ithaca.c`, `Island.c`) plus
shared modules (`io`, `text`, `config`, `commands`, `lifecycle`, `voyages`, `stock`,
`routes`). A single `island` executable represents any of the six islands, depending
only on the configuration file it is given. Phase 1 does not allow sockets, forks,
threads or IPC, so none of them appear in the code.

## 3. Data structures and justification

Every aggregate (`tOdysseusConfig`, `tIthacaConfig`, `tVoyageList`, `tIslandConfig`,
`tRouteList`, `tStockList`) is a small struct, and every list is a dynamic array with a
count and a capacity that grows with `realloc` through a temporary pointer. Nothing is
sized to the provided files (24 voyages, 8 products, 5 candidate routes): a longer
`voyages.dat` or a stock file with a different number of products is loaded correctly
without changing the code.

Each structure has an init function and a destroy function. The destroy function is
safe on a structure that was only partly filled, and counts only include entries that
are completely loaded, so the same cleanup code works after a success or after a
failure halfway through loading.

## 4. Resources/APIs used and why

- `open`/`close`/`read`/`write` for all file and screen I/O, as the project requires.
  `vasprintf` is used only to format a message in memory before writing it; no
  `printf`/`fprintf`/`fgets`-style function is used.
- `sigprocmask` + `signalfd` + `poll` for CTRL+C instead of a handler with a flag. The
  UNIX book (section 12.4) explains the race in a `while (!flag) pause();` loop. With
  SIGINT blocked for the whole process and read through a descriptor in `poll()`, that
  race cannot happen, and since there is no signal handler, no memory is allocated or
  freed inside one.
- `poll()` with an infinite timeout, so Ithaca and Island really sleep until CTRL+C
  instead of busy waiting (measured: 0 CPU ticks over 3 seconds while idle).
- `strtok_r` instead of `strtok`, so tokenizing one line never interferes with another.
- `strtol` instead of `atoi` for numbers, since `atoi` cannot tell an invalid token from
  a real `0`, and cannot detect overflow.

## 5. Problems encountered and solutions

- **Sphragis ownership.** `SPHRAGIS_filter_island_configuration()` frees the names of
  the routes it rejects, so it needs strings it is allowed to free. Our route list also
  needs the names together with the IP and port of each route, which the library does
  not know about. Giving the library our own strings would leave the route list pointing
  at freed memory. Solution: we give the library a temporary array of copies of the
  names, then match each surviving name back to the original route to recover its IP
  and port, and build a new list of valid routes from that.
- **Binary stock layout.** Reading the file straight into
  `struct { char name[100]; int amount; int price; }` depends on how the compiler lays
  out the struct. Instead we read exactly 108 bytes per record and decode the two
  integers byte by byte as little-endian. A record that stops before 108 bytes is
  treated as a corrupted file, not as a product.
- **EOF, blank lines and partial reads.** `read()` can return part of a line, several
  lines, or nothing. We keep the bytes in a growing buffer and only take complete lines
  out of it, and we handle a last line without a newline separately. A read error, the
  end of the file and an empty line are three different cases in the code.
- **CTRL+C while a command is half typed.** If Odysseus only waited on stdin, a CTRL+C
  after typing part of a line would not be noticed. Odysseus uses `poll()` on stdin and
  on the signal descriptor at the same time, so CTRL+C ends the program immediately and
  the unfinished line is discarded.
- **Error paths.** Our first tests only covered runs where everything succeeds. When we
  started making memory allocations and writes fail on purpose, we found four bugs: a
  double free in the island route loader when a route line failed after another route
  had already been stored; an out-of-memory error in the command parser that was shown
  as `Unknown command`; a last command without a newline that was silently lost if
  memory ran out at that moment; and failed writes to the terminal that were ignored.
  We fixed them by giving each route line its own strings (freed only by that line's
  code), adding a separate parser result for memory errors, and making every output
  and end-of-input step return a status. Any such failure now prints an error on
  stderr, frees everything and exits with status 2. We added a test script that makes
  every allocation and every screen write fail, one at a time, and runs each case under
  Valgrind. We also went through the style guide again: system includes moved into
  the headers, and every function got its full header comment.
- A second review found one more error path: when stdout (or stderr) was a pipe whose
  reader had already closed, the first write raised SIGPIPE and the process died
  without cleaning up, because SIGPIPE's default action terminates the process. Our
  fault tests had not caught it: they made `write()` return an error, but a real
  broken pipe sends the signal before `write()` returns. Each program now sets SIGPIPE
  to `SIG_IGN` with `sigaction()` as the first step of `main()`, before any message is
  written, so the write returns `EPIPE` and the normal error handling runs (message on
  stderr if it still works, cleanup, exit 2; exit 1 is kept for wrong arguments). We
  added `tests/pipe_tests.py`, which uses real pipes with the reader closed, both
  before start-up and after the programs are ready, and also runs under Valgrind.

## 6. Block diagram

Three separate executables. In Phase 1 there are no connections between them.

```
                +----------------+
 argv --------> |   Odysseus.c   |  only process with a terminal
                +----------------+
                | config.c/.h    |  loadOdysseusConfig / destroyOdysseusConfig
                | commands.c/.h  |  parseCommand (syntax only, no I/O)
                | lifecycle.c/.h |  ignoreSigpipe / blockSigint / createSigintFd / consumeSignal
                | text.c/.h      |  line buffer, tokenizer, number parsing
                | io.c/.h        |  writeAll / writeString / writeFormatted / safeRead
                +----------------+

                +----------------+
 argv --------> |   Ithaca.c     |  no terminal, waits for CTRL+C
                +----------------+
                | config.c/.h    |  loadIthacaConfig / destroyIthacaConfig
                | voyages.c/.h   |  loadVoyages / destroyVoyageList
                | lifecycle.c/.h, text.c/.h, io.c/.h (shared)
                +----------------+

                +----------------+
 argv --------> |   Island.c     |  no terminal, waits for CTRL+C
                +----------------+
                | config.c/.h    |  loadIslandConfig (+ raw routes)
                | routes.c/.h    |  filterIslandRoutes -> Sphragis library
                | stock.c/.h     |  loadStockList / destroyStockList
                | lifecycle.c/.h, text.c/.h, io.c/.h (shared)
                +----------------+

                lib/sphragis.h, lib/sphragis.o  (provided, linked only into island)
```

## 7. Time spent

(To be completed.)

| Student | Research | Design | Implementation | Testing | Documentation |
| --- | ---: | ---: | ---: | ---: | ---: |
| Salah Ahmed Salaheldin Adly Rashwan | | | | | |

## 8. Conclusions and proposed improvements

The hard part of Phase 1 is not any single feature but being careful everywhere: every
read can be partial, every allocation can fail, and every failure has to free exactly
what was already allocated. None of that shows up in a normal demo run, which is why the
tests that make things fail on purpose were the most useful ones.

For Phase 2, the parser, the loaders and the Sphragis adapter should not need to change
structure. The new work is the state (connected, docked, active voyage) and the network
code, and the command dispatcher can call real handlers next to the existing syntax
checks.

## 9. Theme note

The Nostos System follows the Odyssey: Ithaca is home and gives out the missions,
Odysseus travels to complete voyages and return, and each Island is a place from the
poem (Aeaea is Circe's island, Aeolia is the island of Aeolus, and so on), each with its
own goods and its own known neighbours.

## 10. Bibliography

- OS Project 2026-27 statement, *The Nostos System*, Phase 1 sections.
- La Salle C Style Guide.
- OS Project, Phase 1 official test cases.
- `sphragis.h`, Sphragis library header.
- *UNIX Programming* course notes 2026-27: descriptor I/O, `strtol`/`strtok_r`,
  `sigaction`/`signalfd`/`poll`, EINTR and short reads.
- Course code samples: File Descriptors, Signals, and Select.
