# Test results — Nostos Phase 1

All results below were actually executed in this session on 21 September 2026. Nothing
here is claimed without having been run and its output inspected. Montserrat-specific
verification is explicitly marked pending (see section 7).

## 1. Platform and toolchain

Local Linux target: **WSL2 Ubuntu 24.04.2 LTS**, a genuine GNU/Linux kernel and
userspace (not Windows emulation — `uname -a` reports a real Linux kernel, and the
supplied `lib/sphragis.o` is ELF64 little-endian x86-64, confirmed identical by MD5 to
the original `Phase1/Sphragis libray/sphragis.o`). This is strong evidence but **not**
proof of Montserrat acceptance (different distro/kernel/glibc); see section 7.

```
uname -a: Linux LAPTOP-Q08GCJ0V 6.18.33.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC
          Thu Jun 18 21:54:43 UTC 2026 x86_64 x86_64 x86_64 GNU/Linux
gcc:      gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
valgrind: valgrind-3.22.0
```

## 2. Build

```sh
$ make clean && make all
```

Result: **all three executables build with zero compiler warnings** under
`-Wall -Wextra -std=gnu11`. `island` links successfully against the real, unmodified
`lib/sphragis.o` (confirmed no undefined-reference errors; no guessed extra linker flags
were needed). Verified:

*Environmental note:* building directly from the Windows-mounted `/mnt/c/...` path (a
DrvFs 9p filesystem) occasionally makes `make` print `Clock skew detected` / a
modification-time-in-the-future warning — this is a `make` timestamp-precision quirk of
that specific mount, not a compiler warning about the code (no `gcc` line ever emits a
warning). Confirmed absent when the identical tree is copied to a native Linux
filesystem (`/tmp`) and built there (also used for the fresh-extraction check in §8).

- A second `make all` with nothing changed performs no rebuild (`make: Nothing to be
  done for 'all'.`).
- Touching `include/text.h` correctly triggers a rebuild of every object that includes
  it (directly or transitively) and relinks all three executables (`-MMD -MP`
  dependency tracking).
- `make clean` removes only `build/*.o`, `build/*.d`, and the three executables; MD5 of
  `lib/sphragis.o` and `data/voyages.dat` verified unchanged before/after.
- File type of all three: `ELF 64-bit LSB pie executable, x86-64 ... for GNU/Linux 3.2.0`.

## 3. Functional tests: `tests/run_functional_tests.sh`

Run via `make test`. Result on this platform:

```
=== Summary: 60 passed, 0 failed ===
```

This covers, exactly as captured (not paraphrased):

- The official positive matrix (T p.2): `CONNECT ITHACA`, `LIST VOYAGES`, `ACCEPT 2`,
  `SAIL Aeaea`, `MAP`, `LIST MARKET`, `BUY DriedFish 3`, `SELL Barley 10`, `STATUS`,
  `DELIVER`, `CLAIM`, `cOnNeCt iThAcA` — all `Command OK`.
- The official negative matrix (T p.2): `ACCEPT` → `Usage: ACCEPT <voyage_id>`;
  `BUY DriedFish` → `Usage: BUY <product> <amount>`; `MAP now` → `Usage: MAP`;
  `something else` → `Unknown command`.
- Case-insensitivity, missing/wrong fixed words, missing/extra arguments, prefix
  impostors (`MAPS`, `ACCEPTED 2`, `BUYER X 1` → `Unknown command`), numeric junk,
  nonpositive quantities, ID lexical policy (`0`, `-1`, `+2` rejected; `0002` accepted
  as 2), overflow (`ACCEPT 99999999999999999999` → usage, no wraparound), state
  independence (`ACCEPT 999`, `SAIL Atlantis`, `BUY UnknownProduct 3` → `Command OK`),
  the `BUY MAP` quantity rule (A06) and `SELL MAP 1` (A07), and blank-line handling
  (A08, no output).
- Loader smoke tests: missing CLI args (exit 1), missing config/voyages/stock files
  (exit 2), a genuinely truncated stock file (7 full + 44 partial bytes of an 8th
  record, exit 2, controlled error), a malformed island config missing the
  `--- ROUTES ---` marker (exit 2).
- Loader field-level checks via `tests/loader_check` (see section 5): full Odysseus
  field dump, 24 Ithaca voyages, 8 products for every one of the six real stock files.

Manually run beyond the script (transcripts captured in this session, reproducible with
the commands shown):

- `printf "CONNECT ITHACA\nSTATUS" | ./odysseus configs/odysseus.dat` (no trailing
  newline on the final command) → both processed, `Command OK` twice, exit 0.
- `printf "" | ./odysseus configs/odysseus.dat` (empty stdin) → prints only the
  readiness line and one prompt, exit 0, no hang.
- CTRL+C with a partial, newline-less command already buffered (`CONNECT ITH` sent
  through a FIFO, then real `SIGINT`) → the partial command is silently discarded, clean
  shutdown, exit 0.
- CTRL+C at idle for `ithaca`/`island`/`odysseus` → clean shutdown message, exit 0.
- **Idle CPU check**: `ithaca` backgrounded, `/proc/<pid>/stat` fields 14+15 (utime +
  stime) read before and after a 3-second sleep with the process otherwise untouched:
  **0 ticks consumed in both readings** — confirms a genuinely blocked wait, not a busy
  loop.
- Six real islands (`aeaea`..`thrinacia`) each started against their own config/stock
  pair and terminated with SIGINT: all print `Island <Name> initialized.`,
  `Port capacity: N ships.`, a **library-decided** (not hardcoded) route count, and
  `8 products available.`, then `<Name> closes its port.` on shutdown, exit 0 in every
  case.
- CRLF-normalized copy of `ithaca.dat` (every line's `\n` replaced with `\r\n`) still
  loads all 24 voyages correctly.
- Empty `voyages.dat` → `Ithaca initialized. 0 voyages loaded.`, no crash (zero-record
  policy).
- Empty `stock.db` → `0 products available.`, no crash (zero-record policy).

## 4. Sphragis integration: real, library-decided route filtering

Using `tests/loader_check island <config> <stock>` against the real six authored
fixture configs (each listing all five *other* islands as raw candidates — no topology
assumed or hardcoded):

| Island | Raw candidates | Real Sphragis result | Valid routes (name + endpoint) |
| --- | ---: | ---: | --- |
| Aeaea | 5 | 2 | Scheria 172.16.214.24:8620, Thrinacia 172.16.214.25:8621 |
| Aeolia | 5 | 1 | Scheria 172.16.214.24:8620 |
| Ismarus | 5 | 1 | Scheria 172.16.214.24:8620 |
| Ogygia | 5 | 1 | Thrinacia 172.16.214.25:8621 |
| Scheria | 5 | 3 | Aeaea 172.16.214.20:8621, Aeolia 172.16.214.21:8622, Ismarus 172.16.214.22:8623 |
| Thrinacia | 5 | 2 | Aeaea 172.16.214.20:8621, Ogygia 172.16.214.23:8624 |

Cross-checked as fully **bidirectional and endpoint-consistent** with no manual
correction: Aeaea↔Scheria, Aeaea↔Thrinacia, Aeolia↔Scheria, Ismarus↔Scheria,
Ogygia↔Thrinacia all appear on both sides with matching IP:port. This is direct
evidence that (a) the real `SPHRAGIS_filter_island_configuration()` was actually called
and actually decided the survivor set (a hub topology centered on Scheria/Thrinacia that
was never hardcoded anywhere in this codebase), and (b) the adapter's survivor-matching
never crosses a name with the wrong endpoint.

## 5. Loader field-level evidence: `tests/loader_check`

Built separately (not part of `make all`; see README "Tests" section). Sample output
(full transcripts available by re-running):

```
odysseus.name=Polyphemus
odysseus.storageFolder=./polyphemus_files
odysseus.ithaca=172.16.214.10:8620
odysseus.initialIsland=Aeaea 172.16.214.20:8621
odysseus.gold=250
odysseus.foodCount=2
odysseus.food[0]=Barley 40
odysseus.food[1]=DriedFigs 60
```

```
ithaca.voyageCount=24
ithaca.voyage[0]=id=1 object=AeolusBagOfWinds file=objects/aeolus_bag_of_winds.png dest=Scheria reward=1150
ithaca.voyage[2]=id=3 object=OdysseusBow file=objects/odysseus_bow.png dest=Ismarus reward=1050
ithaca.voyage[23]=id=24 object=AtlasStarAstrolabe file=objects/atlas_star_astrolabe.png dest=Thrinacia reward=900
```

`OdysseusBow` is voyage #3, matching the actual supplied `voyages.dat` order (not the
illustrative statement excerpt where it appears first). The last record matches exactly.

All six stock files decode to exactly 8 products each with correct name/amount/price
(cross-checked independently against a Python `struct.unpack('<ii', ...)` decode of the
raw bytes before any C code ran, e.g. `Aeaea.db` → first record `Barley 140kg 7g/kg`,
last record `EnchantedHerbs 60kg 22g/kg`, matching both the guide's documented fixture
facts and the C loader's own output).

## 6. Valgrind: `tests/run_memcheck.sh`

Run via `make memcheck`. All six cases reported **0 errors, 0 bytes in 0 blocks at
exit** (every heap allocation matched by a free — see exact alloc/free counts below),
and **no application-opened descriptor left open** (only Valgrind's own log-file
descriptor and an inherited `/dev/ptmx` appear in the "FILE DESCRIPTORS" section, both
pre-existing/inherited, not opened by this codebase):

| Case | Path exercised | Allocs / frees | Errors |
| --- | --- | ---: | ---: |
| `odysseus_sigint` | Full 20-command matrix via stdin, then real SIGINT mid-idle | 93 / 93 (EOF run); 28 / 28 (SIGINT-on-partial-line run) | 0 |
| `ithaca_sigint` | Normal init → SIGINT → shutdown | 134 / 134 | 0 |
| `island_sigint` | Real Sphragis filtering → SIGINT → shutdown | 48 / 48 | 0 |
| `odysseus_missing_config` | Config open failure | 0 / 0 | 0 |
| `ithaca_missing_voyages` | Voyages open failure after config load | 8 / 8 | 0 |
| `island_truncated_stock` | Genuinely truncated stock record | 43 / 43 | 0 |

The `island_sigint` case is the most important one to audit: it is the only path that
exercises the real Sphragis adapter's temporary-array ownership handoff under Memcheck,
and it reports zero leaks and zero double-frees.

## 7. What remains unrun / pending

- **Montserrat itself was not accessed in this session.** All results above are from a
  real GNU/Linux (WSL2 Ubuntu 24.04) build/run, which is strong local evidence but not
  proof of acceptance on the actual grading host (different distro/kernel/glibc/`gcc`
  version are possible). Re-running `make clean && make all && make test && make
  memcheck` on Montserrat itself is the outstanding step before calling this
  submission-ready.
- A genuine terminal-generated `SIGINT` (real TTY line discipline sending `Ctrl+C`, not
  a piped byte or `kill -INT`) was not exercised interactively in this session; `kill
  -INT <pid>` sends the identical signal Ithaca/Island/Odysseus observe via their
  `signalfd`, so the code path is the same, but a literal keyboard-driven session is
  still worth doing once on Montserrat.
- Allocation-failure fault injection (e.g. a wrapped `malloc` that fails on the Nth
  call) was **not** performed; only the ordinary error paths reachable by real
  missing/malformed files were tested. Every allocation error path was inspected by
  code review (see `docs/audit-handoff.md`), not exercised under fault injection.
- No design validation meeting with instructors/interns has taken place (assumption
  A02); this implementation is not to be presented as instructor-approved.
- The final `G<group>_F1.tar` has not been produced, since the actual group number is
  not established by this handoff (assumption A15) — see section 8 for the
  fresh-extraction check already performed, which stands in for it functionally.

## 7a. Non-hardcoding check: variable-sized voyages file

`tests/loader_check ithaca configs/ithaca.dat <file>` against a 10-line truncated copy
and a 48-line doubled copy of the real `voyages.dat`:

```
head -10 data/voyages.dat  -> ithaca.voyageCount=10
cat v.dat v.dat            -> ithaca.voyageCount=48
```

Confirms the voyage count (and, by the same array-growth code path, the route/stock
counts) is genuinely file-driven, not hardcoded to the supplied fixture's 24 records.

## 8. Fresh-extraction packaging check (guide step 16) — actually run

```sh
$ tar --exclude=build --exclude=tests/valgrind-logs -cf /tmp/nostos_test.tar .
$ tar tf /tmp/nostos_test.tar | wc -l     # 88 entries
$ mkdir -p /tmp/fresh_extract_test && cd /tmp/fresh_extract_test
$ tar xf /tmp/nostos_test.tar
$ make clean && make all                  # succeeded, zero warnings
$ ./ithaca configs/ithaca.dat data/voyages.dat   # SIGINT after 0.5s
Ithaca initialized. 24 voyages loaded.
Waiting for Odysseus...
Ithaca closes the harbor.
(exit 0)
$ printf 'CONNECT ITHACA\nSTATUS\n' | ./odysseus configs/odysseus.dat
Odysseus Polyphemus is ready to sail.
$ Command OK
$ Command OK
$
```

This ran entirely from `/tmp/fresh_extract_test`, with no absolute-path access back to
`Phase1Code/nostos` or the original `Phase1/`/`All Materials/` directories — a real
stand-in for "extract the submitted tar on a clean machine and build/run it", short of
an actual `G<group>_F1.tar` name (pending the real group number, assumption A15) and
short of doing it on Montserrat itself.
