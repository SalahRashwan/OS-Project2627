# Testing walkthrough — Nostos Phase 1

A step-by-step guide for manually verifying `nostos/` yourself. This complements
`docs/test-results.md` (which records what was *already* run and its actual output) —
use this file to reproduce those results and to explore the build interactively.

Everything below runs in a **WSL Ubuntu terminal**, not PowerShell/cmd — the project
requires real Linux (`signalfd`, `poll`, and the vendor `sphragis.o` won't link under
Windows-native gcc).

## 1. Open a WSL shell and go to the project

In any Windows Terminal tab (PowerShell, cmd, or an existing Ubuntu tab), run:

```powershell
wsl -d Ubuntu --cd "/mnt/c/Users/Salah Rashwan/Desktop/UNI 2027/OS/Project/Phase1Code/nostos"
```

This starts a real Ubuntu Linux shell **inside that same tab**, already sitting in the
`nostos` folder — no `cd` needed afterward. Your prompt changes from something like
`PS C:\...>` to `username@computer:/mnt/c/.../nostos$`; that's your confirmation you're
in. Type `exit` at any point to leave it and return to your normal Windows shell.

(Equivalently: open **Windows Terminal → Ubuntu** from the profile dropdown, or just run
`wsl` from PowerShell/cmd, then `cd` into the folder manually — the one-liner above just
does both in one step.)

## 2. Build

```sh
make clean
make
```

**Expect:** a series of `gcc ...` lines, no warnings, ending with three link lines
producing `odysseus`, `ithaca`, `island`. If you see any `warning:` from `gcc` itself
(not `make`'s own clock-skew notice, which is harmless and specific to the
Windows-mounted filesystem — see `docs/test-results.md` §2), that's a real problem.

## 3. Run Odysseus interactively (the terminal)

```sh
./odysseus configs/odysseus.dat
```

You'll see `Odysseus Polyphemus is ready to sail.` and a `$ ` prompt. Type commands one
at a time and watch the output:

```
CONNECT ITHACA         -> Command OK
LIST VOYAGES           -> Command OK
ACCEPT 2               -> Command OK
ACCEPT                 -> Usage: ACCEPT <voyage_id>
ACCEPT abc              -> Usage: ACCEPT <voyage_id>
BUY MAP 2              -> Usage: BUY <product> <amount>
BUY MAP 1              -> Command OK
gibberish               -> Unknown command
```

Try mixed case (`cOnNeCt iThAcA`), extra tokens (`STATUS now`), and a blank line (just
press Enter — nothing should print). Exit with **CTRL+D** (EOF) or **CTRL+C** — both
should return you to the shell immediately, no hang.

## 4. Run Ithaca and Island (no terminal — just watch them block, then CTRL+C)

```sh
./ithaca configs/ithaca.dat data/voyages.dat
```

You should see `Ithaca initialized. 24 voyages loaded.` then `Waiting for Odysseus...`,
and the shell just sits there — that's correct, it's blocked waiting for a signal.
Press **CTRL+C**: it should immediately print `Ithaca closes the harbor.` and return you
to the shell (check `echo $?` afterward — should be `0`).

Same idea for an island, picking any of the six config/stock pairs:

```sh
./island configs/aeaea.dat data/stocks/Aeaea.db
```

You'll see the init messages including a **route count** — that number comes from the
real Sphragis library, not something hardcoded, so it may not be "2" if you try a
different island. Try a few:

```sh
./island configs/scheria.dat data/stocks/Scheria.db   # busiest hub, 3 routes
```

CTRL+C each one and confirm the `<Name> closes its port.` message and clean return.

## 5. Confirm it's genuinely blocked, not spinning

While an `ithaca` or `island` is running (from step 4, in one terminal), open a
**second** WSL terminal and check CPU usage:

```sh
top -p $(pgrep ithaca)
```

Watch it for a few seconds — CPU% should sit at `0.0`. If it's climbing, something's
busy-waiting and that's a real bug.

## 6. Run the automated test suite

Back in your main terminal:

```sh
make test
```

This first runs the style scan (expect `style_check: 0 finding(s)`), then 100 scripted
checks (official test-sheet cases, the full edge-case matrix, exit statuses, SIGINT
shutdown of every program, and field-by-field loader comparisons) and prints
`Summary: 100 passed, 0 failed` at the end. Any `FAIL:` line shows what was expected
vs. what actually happened.

## 7. Run Valgrind (memory/descriptor leak checking)

```sh
sudo apt install valgrind      # only if not already installed
make memcheck
make faults                    # slow (several minutes): every allocation failure
```

`make memcheck` holds Odysseus's stdin open (so the test really exercises CTRL+C, not
EOF), buffers a partial command, sends `SIGINT`, and requires exit 0; it does the same
for Ithaca and two islands, plus an EOF run and six failure paths. Expect eleven `OK:`
lines. Each log must show `ERROR SUMMARY: 0 errors`, `in use at exit: 0 bytes`, and no
open descriptor other than 0-2 or inherited ones (Valgrind's own log file appears as an
inherited descriptor; that is expected).

`make faults` makes each allocation call fail in turn, and each stdout write fail, and
checks that every program either succeeds completely or exits 2 with an error message,
never crashing, leaking, or misreporting a valid command as `Unknown command`.

## 8. (Optional) Try breaking it on purpose

A few things worth trying by hand, since they're the trickiest paths:

```sh
# Missing file — should print a clear error and exit 2, not hang or crash
./island configs/aeaea.dat data/stocks/does_not_exist.db; echo "exit=$?"

# Truncated binary stock file — should be rejected, not silently read as fewer products
head -c 800 data/stocks/Aeaea.db > /tmp/bad.db
./island configs/aeaea.dat /tmp/bad.db; echo "exit=$?"

# Redirect a file of commands into Odysseus instead of typing
printf "CONNECT ITHACA\nSTATUS\n" | ./odysseus configs/odysseus.dat
```

## If something doesn't match

If any of the above doesn't match what's described here (a `FAIL:` in the test suite, a
nonzero Valgrind error count, a hang, CPU spinning), paste the exact output back for
debugging — don't assume it's expected. See `docs/assumptions.md` if the *disagreement*
is about what a command's output *should* be (some choices, like blank-line handling or
the `BUY MAP` quantity rule, are documented defaults rather than verbatim instructor
requirements, and are easy to change in one place if you get a different ruling).
