#!/bin/sh
# @File: run_fault_tests.sh
# @Purpose: Regression suite for audit findings F1-F4. Uses the test-only
#           fault-injection builds (tests/bin/*-fault, see
#           tests/fault_inject.c) to fail, one at a time, EVERY counted
#           allocation call (malloc/realloc/vasprintf) of each program,
#           and to fail stdout writes, then checks invariants instead of
#           fragile per-index expectations:
#             - the process never dies from a signal or abort;
#             - exit status is 0 (success) or 2 (reported failure);
#             - exit 2 always comes with a stderr diagnostic;
#             - exit 0 always comes with the complete normal output
#               (so a valid command is never turned into
#               "Unknown command" and an EOF command is never dropped);
#             - under Valgrind: no invalid read/write/free, no leak, no
#               application-owned descriptor left open.
#           Usage: make faults [FAULT_VALGRIND=0 to skip Valgrind]
#           Exit status: 0 = all passed, 1 = failure, 77 = Valgrind
#           requested but not installed (checks NOT run).
# @Author: Salah Ahmed Salaheldin Adly Rashwan
# @Date: 2026-09-26
set -u

cd "$(dirname "$0")/.."
BIN=tests/bin
LOG_DIR=tests/fault-logs
USE_VG="${FAULT_VALGRIND:-1}"
PASS=0
FAIL=0
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT INT TERM
mkdir -p "$LOG_DIR"
rm -f "$LOG_DIR"/*.log

if [ "$USE_VG" = "1" ] && ! command -v valgrind >/dev/null 2>&1; then
    echo "NOT RUN: valgrind not installed; rerun with FAULT_VALGRIND=0 or install it."
    exit 77
fi
for b in odysseus-fault ithaca-fault island-fault; do
    if [ ! -x "$BIN/$b" ]; then
        echo "FAIL: missing $BIN/$b (run make faults)"
        exit 1
    fi
done

fail() {
    FAIL=$((FAIL + 1))
    echo "FAIL: $1 -- $2"
}

# @Name: vg_log_clean
# @Def: Succeeds when a Valgrind log shows no errors, no bytes in use at
#       exit, and no open descriptor beyond 0-2 that was not inherited.
# @Arg: $1 = log path.
vg_log_clean() {
    grep -q "ERROR SUMMARY: 0 errors" "$1" || return 1
    grep -q "in use at exit: 0 bytes in 0 blocks" "$1" || return 1
    extra=$(awk '
        /Open (file|AF_[A-Z0-9]+ socket) descriptor [0-9]+/ {
            if (fd != "" && !inh) n++
            match($0, /descriptor [0-9]+/); fd = substr($0, RSTART + 11, RLENGTH - 11) + 0
            inh = (fd <= 2); next
        }
        /<inherited from parent>/ { inh = 1 }
        END { if (fd != "" && !inh) n++; print n + 0 }' "$1")
    [ "$extra" = "0" ]
}

# @Name: run_case
# @Def: Runs one program instance with the given fault environment. If
#       it is still alive once its readiness text appears (server
#       programs), SIGINT is sent. Stores status/stdout/stderr in $WORK.
# @Arg: $1 = case label, $2 = readiness text ("" = none, stdin driven),
#       $3 = stdin file, rest = command.
run_case() {
    label="$1"; ready="$2"; input="$3"; shift 3
    log="$LOG_DIR/$label.log"
    if [ "$USE_VG" = "1" ]; then
        set -- valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes \
            --log-file="$log" "$@"
    fi
    "$@" <"$input" >"$WORK/out" 2>"$WORK/err" &
    pid=$!
    tries=0
    while kill -0 "$pid" 2>/dev/null && [ "$tries" -lt 400 ]; do
        if [ -n "$ready" ] && grep -qF "$ready" "$WORK/out"; then
            kill -INT "$pid"
            ready=""
        fi
        sleep 0.05; tries=$((tries + 1))
    done
    if kill -0 "$pid" 2>/dev/null; then
        kill -KILL "$pid"
        echo timeout >>"$WORK/err"
    fi
    wait "$pid"
    STATUS=$?
}

# @Name: check_case
# @Def: Applies the common invariants to the last run_case.
# @Arg: $1 = label, $2 = grep -c pattern for success, $3 = count needed.
check_case() {
    label="$1"; pattern="$2"; need="$3"
    got=$(grep -c -- "$pattern" "$WORK/out")
    if [ "$STATUS" = "0" ]; then
        if [ "$got" != "$need" ] || [ -s "$WORK/err" ]; then
            fail "$label" "exit 0 but incomplete output (found $got/$need '$pattern') stderr=[$(cat "$WORK/err")]"
            return
        fi
    elif [ "$STATUS" = "2" ]; then
        if [ ! -s "$WORK/err" ]; then
            fail "$label" "exit 2 without a stderr diagnostic"
            return
        fi
    else
        fail "$label" "abnormal exit status $STATUS stderr=[$(head -c 300 "$WORK/err")]"
        return
    fi
    if grep -q "Unknown command" "$WORK/out"; then
        fail "$label" "valid command reported as Unknown command"
        return
    fi
    if [ "$USE_VG" = "1" ] && ! vg_log_clean "$LOG_DIR/$label.log"; then
        fail "$label" "Valgrind reported a problem (see $LOG_DIR/$label.log)"
        return
    fi
    PASS=$((PASS + 1))
    rm -f "$LOG_DIR/$label.log"
}

# @Name: alloc_calls
# @Def: Calibration run without faults; prints the number of counted
#       allocation calls the scenario performs.
alloc_calls() {
    ready="$1"; input="$2"; shift 2
    NOSTOS_FAULT_REPORT=1 "$@" <"$input" >"$WORK/cal_out" 2>"$WORK/cal_err" &
    cpid=$!
    tries=0
    while kill -0 "$cpid" 2>/dev/null && [ "$tries" -lt 200 ]; do
        if [ -n "$ready" ] && grep -qF "$ready" "$WORK/cal_out"; then kill -INT "$cpid"; ready=""; fi
        sleep 0.05; tries=$((tries + 1))
    done
    wait "$cpid"
    sed -n 's/^alloc_calls=//p' "$WORK/cal_err"
}

# @Name: sweep
# @Def: Fails every allocation call index 1..N of one scenario.
# @Arg: $1 = scenario name, $2 = readiness text, $3 = stdin file,
#       $4 = success pattern, $5 = success count, rest = command.
sweep() {
    name="$1"; ready="$2"; input="$3"; pattern="$4"; need="$5"; shift 5
    total=$(alloc_calls "$ready" "$input" "$@")
    if [ -z "$total" ] || [ "$total" -lt 1 ]; then
        fail "$name" "calibration failed"
        return
    fi
    echo "--- $name: failing each of $total allocation calls"
    n=1
    while [ "$n" -le "$total" ]; do
        NOSTOS_FAIL_ALLOC=$n run_case "${name}_alloc$n" "$ready" "$input" "$@"
        check_case "${name}_alloc$n" "$pattern" "$need"
        n=$((n + 1))
    done
}

# @Name: write_sweep
# @Def: Fails stdout writes from index 1..N onward.
# @Arg: $1 = name, $2 = N, $3 = readiness, $4 = stdin, $5 = success
#       pattern, $6 = success count, rest = command.
write_sweep() {
    name="$1"; count="$2"; ready="$3"; input="$4"; pattern="$5"; need="$6"; shift 6
    echo "--- $name: failing stdout from write 1..$count onward"
    k=1
    while [ "$k" -le "$count" ]; do
        NOSTOS_FAIL_STDOUT_WRITE=$k run_case "${name}_write$k" "$ready" "$input" "$@"
        if [ "$STATUS" != "2" ]; then
            fail "${name}_write$k" "expected exit 2 after a failed stdout write, got $STATUS"
        else
            check_case "${name}_write$k" "$pattern" "$need"
        fi
        k=$((k + 1))
    done
}

: >"$WORK/empty"
printf 'MAP\n' >"$WORK/map_nl"
printf 'MAP' >"$WORK/map_eof"
printf 'SAIL Aeaea\n' >"$WORK/sail_nl"
printf 'BUY DriedFish 3\nSTATUS' >"$WORK/buy_status_eof"
head -n 6 configs/aeaea.dat >"$WORK/bad_second_route.dat"
echo "broken" >>"$WORK/bad_second_route.dat"

echo "=== F2/F3: Odysseus parser and EOF allocation failures ==="
sweep ody_map_nl "" "$WORK/map_nl" "Command OK" 1 $BIN/odysseus-fault configs/odysseus.dat
sweep ody_map_eof "" "$WORK/map_eof" "Command OK" 1 $BIN/odysseus-fault configs/odysseus.dat
sweep ody_sail "" "$WORK/sail_nl" "Command OK" 1 $BIN/odysseus-fault configs/odysseus.dat
sweep ody_buy_status_eof "" "$WORK/buy_status_eof" "Command OK" 2 $BIN/odysseus-fault configs/odysseus.dat

echo "=== F1: Island allocation failures (valid config, all routes) ==="
sweep island_aeaea "products available." "$WORK/empty" "closes its port." 1 \
    $BIN/island-fault configs/aeaea.dat data/stocks/Aeaea.db
sweep island_scheria "products available." "$WORK/empty" "closes its port." 1 \
    $BIN/island-fault configs/scheria.dat data/stocks/Scheria.db

echo "=== F1: Island malformed route after a stored route ==="
run_case island_bad_second_route "" "$WORK/empty" $BIN/island-fault "$WORK/bad_second_route.dat" data/stocks/Aeaea.db
if [ "$STATUS" = "2" ]; then check_case island_bad_second_route "never" 0; else fail island_bad_second_route "status $STATUS"; fi

echo "=== Ithaca allocation failures ==="
sweep ithaca "Waiting for Odysseus..." "$WORK/empty" "Ithaca closes the harbor." 1 \
    $BIN/ithaca-fault configs/ithaca.dat data/voyages.dat

echo "=== F4: stdout write failures ==="
printf 'MAP\nSTATUS\n' >"$WORK/two_cmds"
write_sweep ody_output 6 "" "$WORK/two_cmds" "Command OK" 2 $BIN/odysseus-fault configs/odysseus.dat
write_sweep ithaca_output 3 "Waiting for Odysseus..." "$WORK/empty" "never" 0 \
    $BIN/ithaca-fault configs/ithaca.dat data/voyages.dat
write_sweep island_output 5 "products available." "$WORK/empty" "never" 0 \
    $BIN/island-fault configs/aeaea.dat data/stocks/Aeaea.db

echo ""
echo "=== Fault summary: $PASS passed, $FAIL failed (valgrind=$USE_VG) ==="
if [ "$FAIL" != "0" ]; then
    echo "Kept logs of failing cases in $LOG_DIR/"
    exit 1
fi
exit 0
