#!/bin/sh
# @File: run_functional_tests.sh
# @Purpose: Reproducible functional test driver for the three built
#           executables. Shell-only orchestration (per the guide,
#           orchestration scripts may use ordinary shell tools even
#           though the C runtime itself may not); it launches the real
#           binaries and compares real captured output, it does not
#           preprocess data for them or substitute for their own logic.
# @Author: Salah Ahmed Salaheldin Adly Rashwan
# @Date: 2026-09-21
set -u

cd "$(dirname "$0")/.."
PASS=0
FAIL=0
FAILED_NAMES=""

# @Name: expect
# @Def: Runs one Odysseus command batch through stdin and compares the
#       captured stdout (after the initial readiness line) to an exact
#       expected block.
# @Arg: $1 = test name, $2 = command input (may be multi-line),
#       $3 = expected output block (may be multi-line).
expect() {
    name="$1"
    input="$2"
    expected="$3"
    raw=$(printf '%s\n' "$input" | ./odysseus configs/odysseus.dat 2>/tmp/nostos_stderr.txt)
    # Drop the readiness line, then strip every "$ " prompt occurrence
    # (prompts are not newline-terminated, so consecutive blank-line
    # reprompts can share one physical output line with no separator).
    actual=$(printf '%s' "$raw" | sed '1d' | sed 's/\$ //g')
    if [ "$actual" = "$expected" ]; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
        FAILED_NAMES="$FAILED_NAMES $name"
        echo "FAIL: $name"
        echo "  expected: $(printf '%s' "$expected" | tr '\n' '|')"
        echo "  actual:   $(printf '%s' "$actual" | tr '\n' '|')"
    fi
}

echo "=== Official positive command cases (T p.2) ==="
expect "official_positive" \
"CONNECT ITHACA
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
cOnNeCt iThAcA" \
"Command OK
Command OK
Command OK
Command OK
Command OK
Command OK
Command OK
Command OK
Command OK
Command OK
Command OK
Command OK"

echo "=== Official negative cases (T p.2) ==="
expect "official_accept_usage" "ACCEPT" "Usage: ACCEPT <voyage_id>"
expect "official_buy_usage" "BUY DriedFish" "Usage: BUY <product> <amount>"
expect "official_map_usage" "MAP now" "Usage: MAP"
expect "official_unknown" "something else" "Unknown command"

echo "=== Expanded case-insensitivity ==="
expect "case_list_market" "list market" "Command OK"
expect "case_status" "sTaTuS" "Command OK"
expect "case_buy" "bUy DriedFish 3" "Command OK"
expect "case_buy_map" "buy map 1" "Command OK"

echo "=== Missing fixed word ==="
expect "missing_connect_word" "CONNECT" "Usage: CONNECT ITHACA"
expect "missing_list_word" "LIST" "Usage: LIST VOYAGES
Usage: LIST MARKET"

echo "=== Wrong fixed word ==="
expect "wrong_connect_word" "CONNECT Aeaea" "Usage: CONNECT ITHACA"
expect "wrong_list_word" "LIST cargo" "Usage: LIST VOYAGES
Usage: LIST MARKET"

echo "=== Missing argument ==="
expect "missing_sail_arg" "SAIL" "Usage: SAIL <island>"
expect "missing_buy_args" "BUY" "Usage: BUY <product> <amount>"
expect "missing_sell_arg" "SELL Barley" "Usage: SELL <product> <amount>"

echo "=== Extra argument ==="
expect "extra_connect" "CONNECT ITHACA now" "Usage: CONNECT ITHACA"
expect "extra_list_voyages" "LIST VOYAGES 2" "Usage: LIST VOYAGES"
expect "extra_accept" "ACCEPT 2 3" "Usage: ACCEPT <voyage_id>"
expect "extra_sail" "SAIL Aeaea now" "Usage: SAIL <island>"
expect "extra_status" "STATUS now" "Usage: STATUS"
expect "extra_deliver" "DELIVER x" "Usage: DELIVER"
expect "extra_claim" "CLAIM x" "Usage: CLAIM"
expect "extra_buy" "BUY Barley 2 x" "Usage: BUY <product> <amount>"

echo "=== Prefix impostors ==="
expect "impostor_maps" "MAPS" "Unknown command"
expect "impostor_accepted" "ACCEPTED 2" "Unknown command"
expect "impostor_buyer" "BUYER X 1" "Unknown command"

echo "=== Numeric junk ==="
expect "junk_accept_alpha" "ACCEPT abc" "Usage: ACCEPT <voyage_id>"
expect "junk_accept_mixed" "ACCEPT 2x" "Usage: ACCEPT <voyage_id>"
expect "junk_buy_unit" "BUY Barley 3kg" "Usage: BUY <product> <amount>"
expect "junk_sell_decimal" "SELL Barley 1.5" "Usage: SELL <product> <amount>"

echo "=== Nonpositive quantity ==="
expect "zero_buy" "BUY Barley 0" "Usage: BUY <product> <amount>"
expect "negative_sell" "SELL Barley -2" "Usage: SELL <product> <amount>"

echo "=== ID lexical policy ==="
expect "accept_zero" "ACCEPT 0" "Usage: ACCEPT <voyage_id>"
expect "accept_negative" "ACCEPT -1" "Usage: ACCEPT <voyage_id>"
expect "accept_plus" "ACCEPT +2" "Usage: ACCEPT <voyage_id>"

echo "=== Numeric representation (leading zeros) ==="
expect "accept_leading_zeros" "ACCEPT 0002" "Command OK"
expect "buy_leading_zeros" "BUY Barley 0003" "Command OK"

echo "=== Overflow ==="
expect "accept_overflow" "ACCEPT 99999999999999999999" "Usage: ACCEPT <voyage_id>"

echo "=== State independence (syntax-only Phase 1) ==="
expect "accept_absent_id" "ACCEPT 999" "Command OK"
expect "sail_unknown_island" "SAIL Atlantis" "Command OK"
expect "buy_unknown_product" "BUY UnknownProduct 3" "Command OK"

echo "=== Map special form (assumptions A06/A07) ==="
expect "buy_map_1" "BUY MAP 1" "Command OK"
expect "buy_map_2" "BUY MAP 2" "Usage: BUY <product> <amount>"
expect "sell_map_1" "SELL MAP 1" "Command OK"

echo "=== Empty input (assumption A08) ==="
expect "blank_line" "" ""

echo ""
echo "=== Loader smoke tests ==="
run_loader_test() {
    name="$1"; shift
    if "$@" >/tmp/nostos_loader_out.txt 2>&1; then
        actual_exit=0
    else
        actual_exit=$?
    fi
    expected_exit="$LOADER_EXPECTED_EXIT"
    if [ "$actual_exit" = "$expected_exit" ]; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
        FAILED_NAMES="$FAILED_NAMES $name"
        echo "FAIL: $name (expected exit $expected_exit, got $actual_exit)"
        cat /tmp/nostos_loader_out.txt
    fi
}

LOADER_EXPECTED_EXIT=1
run_loader_test "odysseus_no_args" ./odysseus
LOADER_EXPECTED_EXIT=2
run_loader_test "odysseus_missing_config" ./odysseus configs/does_not_exist.dat
run_loader_test "ithaca_missing_config" ./ithaca configs/does_not_exist.dat data/voyages.dat
run_loader_test "island_missing_stock" ./island configs/aeaea.dat data/stocks/does_not_exist.db

head -c 800 data/stocks/Aeaea.db > /tmp/nostos_truncated.db
run_loader_test "island_truncated_stock" ./island configs/aeaea.dat /tmp/nostos_truncated.db

head -4 configs/aeaea.dat > /tmp/nostos_bad_island.dat
run_loader_test "island_missing_marker" ./island /tmp/nostos_bad_island.dat data/stocks/Aeaea.db

echo ""
echo "=== Loader field-level checks (tests/loader_check) ==="
if [ -x tests/loader_check ]; then
    if [ "$(./tests/loader_check odysseus configs/odysseus.dat | grep -c '^odysseus\.')" = "8" ]; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
        FAILED_NAMES="$FAILED_NAMES odysseus_field_count"
        echo "FAIL: odysseus_field_count"
    fi
    VOYAGE_COUNT=$(./tests/loader_check ithaca configs/ithaca.dat data/voyages.dat | grep -c '^ithaca\.voyage\[')
    if [ "$VOYAGE_COUNT" = "24" ]; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
        FAILED_NAMES="$FAILED_NAMES ithaca_voyage_count"
        echo "FAIL: ithaca_voyage_count (got $VOYAGE_COUNT)"
    fi
    for isl in Aeaea Aeolia Ismarus Ogygia Scheria Thrinacia; do
        lower=$(echo "$isl" | tr 'A-Z' 'a-z')
        PRODUCT_COUNT=$(./tests/loader_check island configs/$lower.dat data/stocks/$isl.db | grep -c '^island\.product\[')
        if [ "$PRODUCT_COUNT" = "8" ]; then
            PASS=$((PASS + 1))
        else
            FAIL=$((FAIL + 1))
            FAILED_NAMES="$FAILED_NAMES stock_${lower}_count"
            echo "FAIL: stock_${lower}_count (got $PRODUCT_COUNT)"
        fi
    done
else
    echo "SKIP: tests/loader_check not built (see tests/README or build manually)"
fi

echo ""
echo "=== Summary: $PASS passed, $FAIL failed ==="
if [ "$FAIL" != "0" ]; then
    echo "Failed tests:$FAILED_NAMES"
    exit 1
fi
exit 0
