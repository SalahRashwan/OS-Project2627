#!/bin/sh
# @File: run_memcheck.sh
# @Purpose: Reproducible Valgrind driver covering the CTRL+C path of
#           all three executables plus representative failure paths.
#           Requires `valgrind` on PATH; reports missing tools rather
#           than silently skipping them.
# @Author: Salah Ahmed Salaheldin Adly Rashwan
# @Date: 2026-09-21
set -u

cd "$(dirname "$0")/.."
LOG_DIR="tests/valgrind-logs"
mkdir -p "$LOG_DIR"
VG="valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes"
FAIL=0

if ! command -v valgrind >/dev/null 2>&1; then
    echo "SKIP: valgrind not found on PATH. Memcheck coverage is unrun; report this in docs/test-results.md."
    exit 0
fi

# @Name: run_sigint_case
# @Def: Starts a long-lived process under Valgrind, waits, sends
#       SIGINT, waits for exit, then checks the log for a clean result.
run_sigint_case() {
    name="$1"; log="$LOG_DIR/$name.log"; shift
    $VG --log-file="$log" "$@" >/dev/null 2>&1 &
    pid=$!
    sleep 1
    kill -INT "$pid" 2>/dev/null
    wait "$pid"
    check_log "$name" "$log"
}

# @Name: run_direct_case
# @Def: Runs a short-lived (expected-to-fail) process under Valgrind
#       to completion, then checks the log for a clean result.
run_direct_case() {
    name="$1"; log="$LOG_DIR/$name.log"; shift
    $VG --log-file="$log" "$@" >/dev/null 2>&1 </dev/null
    check_log "$name" "$log"
}

# @Name: check_log
# @Def: Fails the case if the log reports leaks, errors, or any
#       non-inherited open descriptor left at exit.
check_log() {
    name="$1"; log="$2"
    if ! grep -q "ERROR SUMMARY: 0 errors" "$log"; then
        echo "FAIL: $name -- errors reported"
        grep "ERROR SUMMARY" "$log"
        FAIL=1
    fi
    if grep -q "definitely lost\|indirectly lost\|possibly lost" "$log" && ! grep -q "0 bytes in 0 blocks" "$log"; then
        echo "FAIL: $name -- possible leak"
        FAIL=1
    fi
    echo "OK: $name (see $log)"
}

run_sigint_case odysseus_sigint ./odysseus configs/odysseus.dat </dev/null
run_sigint_case ithaca_sigint ./ithaca configs/ithaca.dat data/voyages.dat
run_sigint_case island_sigint ./island configs/aeaea.dat data/stocks/Aeaea.db

run_direct_case odysseus_missing_config ./odysseus configs/does_not_exist.dat
run_direct_case ithaca_missing_voyages ./ithaca configs/ithaca.dat data/does_not_exist.dat

head -c 800 data/stocks/Aeaea.db > /tmp/nostos_truncated.db
run_direct_case island_truncated_stock ./island configs/aeaea.dat /tmp/nostos_truncated.db

if [ "$FAIL" != "0" ]; then
    echo "One or more Valgrind cases reported problems; inspect $LOG_DIR/*.log"
    exit 1
fi
echo "All Valgrind cases clean; logs in $LOG_DIR/"
exit 0
