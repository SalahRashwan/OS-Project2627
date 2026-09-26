#!/bin/sh
# @File: run_memcheck.sh
# @Purpose: Reproducible Valgrind suite (memcheck + descriptor tracking)
#           for all three programs: real SIGINT shutdowns (Odysseus with
#           stdin held open and a partial command buffered, so the case
#           cannot silently degrade into an EOF run), an EOF shutdown,
#           and representative initialization failures. Every case
#           asserts the process exit status, a bounded completion time,
#           and a clean log: 0 errors, 0 bytes in use at exit, and no
#           open descriptor other than 0-2 or ones inherited from the
#           parent.
#           Usage: make memcheck
#           Exit status: 0 = all clean, 1 = at least one failure,
#           77 = valgrind not installed (NOTHING was checked).
# @Author: Salah Ahmed Salaheldin Adly Rashwan
# @Date: 2026-09-26
set -u

cd "$(dirname "$0")/.."
LOG_DIR="tests/valgrind-logs"
PASS=0
FAIL=0
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT INT TERM

if ! command -v valgrind >/dev/null 2>&1; then
    echo "NOT RUN: valgrind not found on PATH. No memory/descriptor check was performed."
    exit 77
fi
mkdir -p "$LOG_DIR"
rm -f "$LOG_DIR"/*.log
VG="valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes"

# @Name: vg_log_clean
# @Def: Prints the reason and fails when a Valgrind log shows errors,
#       bytes in use at exit, or a non-inherited descriptor beyond 0-2.
# @Arg: $1 = log path.
vg_log_clean() {
    if ! grep -q "ERROR SUMMARY: 0 errors" "$1"; then
        grep "ERROR SUMMARY" "$1"; return 1
    fi
    if ! grep -q "in use at exit: 0 bytes in 0 blocks" "$1"; then
        grep "in use at exit" "$1"; return 1
    fi
    extra=$(awk '
        /Open (file|AF_[A-Z0-9]+ socket) descriptor [0-9]+/ {
            if (fd != "" && !inh) n++
            match($0, /descriptor [0-9]+/); fd = substr($0, RSTART + 11, RLENGTH - 11) + 0
            inh = (fd <= 2); next
        }
        /<inherited from parent>/ { inh = 1 }
        END { if (fd != "" && !inh) n++; print n + 0 }' "$1")
    if [ "$extra" != "0" ]; then
        echo "$extra application-owned descriptor(s) left open"; return 1
    fi
    return 0
}

# @Name: finish
# @Def: Records a case result from its status, expected status, and log.
# @Arg: $1 = name, $2 = actual status, $3 = expected status, $4 = extra
#       condition result (0 = ok).
finish() {
    if [ "$2" != "$3" ]; then
        echo "FAIL: $1 -- exit status $2, expected $3"; FAIL=$((FAIL + 1))
    elif [ "$4" != "0" ]; then
        echo "FAIL: $1 -- unexpected program output (see $WORK)"; FAIL=$((FAIL + 1))
    elif ! reason=$(vg_log_clean "$LOG_DIR/$1.log"); then
        echo "FAIL: $1 -- $reason (see $LOG_DIR/$1.log)"; FAIL=$((FAIL + 1))
    else
        echo "OK: $1 (status $2, log $LOG_DIR/$1.log)"; PASS=$((PASS + 1))
    fi
}

# @Name: wait_bounded
# @Def: Waits up to $2 * 0.1 s for process $1 to exit; kills it after.
# @Ret: exit status of the process (137 if it had to be killed).
wait_bounded() {
    tries=0
    while kill -0 "$1" 2>/dev/null && [ "$tries" -lt "$2" ]; do
        sleep 0.1; tries=$((tries + 1))
    done
    if kill -0 "$1" 2>/dev/null; then
        echo "  (process $1 did not finish in time; killed)"
        kill -KILL "$1"
    fi
    wait "$1"
}

# @Name: wait_for_text
# @Def: Waits up to 30 s for a fixed string to appear in a file.
wait_for_text() {
    tries=0
    while ! grep -qF "$1" "$2" && [ "$tries" -lt 300 ]; do
        sleep 0.1; tries=$((tries + 1))
    done
    grep -qF "$1" "$2"
}

# @Name: run_server_sigint
# @Def: Runs Ithaca/Island under Valgrind, waits for readiness, checks
#       it is alive, sends SIGINT, waits (bounded), and requires exit 0
#       plus the shutdown line.
# @Arg: $1 = case name, $2 = readiness text, $3 = shutdown text, rest =
#       command.
run_server_sigint() {
    name="$1"; ready="$2"; bye="$3"; shift 3
    $VG --log-file="$LOG_DIR/$name.log" "$@" </dev/null >"$WORK/$name.out" 2>&1 &
    pid=$!
    ok=1
    if wait_for_text "$ready" "$WORK/$name.out" && kill -0 "$pid" 2>/dev/null; then
        kill -INT "$pid"
        ok=0
    fi
    wait_bounded "$pid" 300
    status=$?
    if [ "$ok" = "0" ] && ! grep -qxF "$bye" "$WORK/$name.out"; then
        ok=1
    fi
    finish "$name" "$status" 0 "$ok"
}

# @Name: run_direct
# @Def: Runs a short-lived case under Valgrind with a given stdin file
#       and requires an exact exit status.
# @Arg: $1 = name, $2 = expected status, $3 = stdin file, rest = command.
run_direct() {
    name="$1"; want="$2"; input="$3"; shift 3
    $VG --log-file="$LOG_DIR/$name.log" "$@" <"$input" >"$WORK/$name.out" 2>&1 &
    pid=$!
    wait_bounded "$pid" 300
    finish "$name" "$?" "$want" 0
}

echo "=== SIGINT shutdown (all three programs) ==="
# Odysseus: stdin is a FIFO that this script keeps open, so EOF can never
# end the process; a partial command ("STAT") is buffered before SIGINT.
mkfifo "$WORK/stdin.fifo"
$VG --log-file="$LOG_DIR/odysseus_sigint_partial.log" ./odysseus configs/odysseus.dat \
    <"$WORK/stdin.fifo" >"$WORK/ody.out" 2>&1 &
pid=$!
exec 3>"$WORK/stdin.fifo"
ok=1
if wait_for_text "ready to sail" "$WORK/ody.out"; then
    printf 'MAP\nSTAT' >&3
    if wait_for_text "Command OK" "$WORK/ody.out" && kill -0 "$pid" 2>/dev/null; then
        kill -INT "$pid"
        ok=0
    fi
fi
wait_bounded "$pid" 300
status=$?
exec 3>&-
if [ "$ok" = "0" ] && [ "$(grep -c 'Command OK' "$WORK/ody.out")" != "1" ]; then
    ok=1
fi
finish odysseus_sigint_partial "$status" 0 "$ok"

run_server_sigint ithaca_sigint "Waiting for Odysseus..." "Ithaca closes the harbor." \
    ./ithaca configs/ithaca.dat data/voyages.dat
run_server_sigint island_aeaea_sigint "products available." "Aeaea closes its port." \
    ./island configs/aeaea.dat data/stocks/Aeaea.db
run_server_sigint island_scheria_sigint "products available." "Scheria closes its port." \
    ./island configs/scheria.dat data/stocks/Scheria.db

echo "=== EOF shutdown ==="
printf 'CONNECT ITHACA\nBUY DriedFish 3\nnonsense\nSTATUS' >"$WORK/cmds"
run_direct odysseus_eof 0 "$WORK/cmds" ./odysseus configs/odysseus.dat

echo "=== Initialization failures ==="
: >"$WORK/empty"
head -c 800 data/stocks/Aeaea.db >"$WORK/truncated.db"
head -n 6 configs/aeaea.dat >"$WORK/bad_second_route.dat"
echo "broken" >>"$WORK/bad_second_route.dat"
head -n 5 configs/odysseus.dat >"$WORK/short_odysseus.dat"
run_direct odysseus_bad_args 1 "$WORK/empty" ./odysseus
run_direct odysseus_missing_config 2 "$WORK/empty" ./odysseus configs/does_not_exist.dat
run_direct odysseus_truncated_config 2 "$WORK/empty" ./odysseus "$WORK/short_odysseus.dat"
run_direct ithaca_missing_voyages 2 "$WORK/empty" ./ithaca configs/ithaca.dat data/does_not_exist.dat
run_direct island_truncated_stock 2 "$WORK/empty" ./island configs/aeaea.dat "$WORK/truncated.db"
run_direct island_bad_second_route 2 "$WORK/empty" ./island "$WORK/bad_second_route.dat" data/stocks/Aeaea.db

echo ""
echo "=== Memcheck summary: $PASS clean, $FAIL failed; logs in $LOG_DIR/ ==="
if [ "$FAIL" != "0" ]; then
    exit 1
fi
exit 0
