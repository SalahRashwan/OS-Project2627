#!/bin/sh
# @File: run_functional_tests.sh
# @Purpose: Reproducible functional suite for the three built programs.
#           Shell-only orchestration: it runs the real binaries and the
#           real loaders (tests/bin/loader_check) and compares them with
#           values decoded independently from the input files by awk/od.
#           Every required component (binaries, loader_check) must exist;
#           a missing one is a FAILURE, never a skip.
#           Usage: make test   (or ./tests/run_functional_tests.sh after
#           make all tests/bin/loader_check)
#           Exit status: 0 = all passed, 1 = at least one failure.
# @Author: Salah Ahmed Salaheldin Adly Rashwan
# @Date: 2026-09-26
set -u

cd "$(dirname "$0")/.."
PASS=0
FAIL=0
FAILED_NAMES=""
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT INT TERM
CHECKER=tests/bin/loader_check
ISLANDS="Aeaea Aeolia Ismarus Ogygia Scheria Thrinacia"

# @Name: pass / fail
# @Def: Record one test outcome; fail prints the reason.
pass() { PASS=$((PASS + 1)); }
fail() {
    FAIL=$((FAIL + 1))
    FAILED_NAMES="$FAILED_NAMES $1"
    echo "FAIL: $1 -- $2"
}

for required in odysseus ithaca island "$CHECKER"; do
    if [ ! -x "$required" ]; then
        fail "prerequisite_$required" "missing executable (run make all tests/bin/loader_check)"
    fi
done

# @Name: expect
# @Def: Runs one Odysseus command batch through stdin (terminated by
#       EOF) and requires exit status 0, empty stderr, and stdout equal
#       to the expected block after the readiness line and prompts.
# @Arg: $1 = test name, $2 = input (printf format), $3 = expected block.
expect() {
    printf "$2" | ./odysseus configs/odysseus.dat >"$WORK/out" 2>"$WORK/err"
    status=$?
    actual=$(sed '1d' "$WORK/out" | sed 's/\$ //g')
    if [ "$status" != "0" ]; then
        fail "$1" "exit status $status"
    elif [ -s "$WORK/err" ]; then
        fail "$1" "unexpected stderr: $(cat "$WORK/err")"
    elif [ "$actual" != "$3" ]; then
        fail "$1" "expected [$(printf '%s' "$3" | tr '\n' '|')] got [$(printf '%s' "$actual" | tr '\n' '|')]"
    else
        pass
    fi
}

echo "=== Odysseus readiness line ==="
printf '' | ./odysseus configs/odysseus.dat >"$WORK/out" 2>&1
NAME=$(sed -n '1p' configs/odysseus.dat | tr -d '\r')
if [ "$(head -n 1 "$WORK/out")" = "Odysseus $NAME is ready to sail." ]; then pass; else fail readiness "$(head -n 1 "$WORK/out")"; fi

echo "=== Official positive command cases (T p.2) ==="
expect official_positive 'CONNECT ITHACA\nLIST VOYAGES\nACCEPT 2\nSAIL Aeaea\nMAP\nLIST MARKET\nBUY DriedFish 3\nSELL Barley 10\nSTATUS\nDELIVER\nCLAIM\ncOnNeCt iThAcA\n' \
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
expect official_accept_usage 'ACCEPT\n' "Usage: ACCEPT <voyage_id>"
expect official_buy_usage 'BUY DriedFish\n' "Usage: BUY <product> <amount>"
expect official_map_usage 'MAP now\n' "Usage: MAP"
expect official_unknown 'something else\n' "Unknown command"

echo "=== Case-insensitivity ==="
expect case_list_market 'list market\n' "Command OK"
expect case_status 'sTaTuS\n' "Command OK"
expect case_buy 'bUy DriedFish 3\n' "Command OK"
expect case_buy_map 'buy map 1\n' "Command OK"

echo "=== Missing / wrong fixed word ==="
expect missing_connect_word 'CONNECT\n' "Usage: CONNECT ITHACA"
expect missing_list_word 'LIST\n' "Usage: LIST VOYAGES
Usage: LIST MARKET"
expect wrong_connect_word 'CONNECT Aeaea\n' "Usage: CONNECT ITHACA"
expect wrong_list_word 'LIST cargo\n' "Usage: LIST VOYAGES
Usage: LIST MARKET"

echo "=== Missing argument ==="
expect missing_sail_arg 'SAIL\n' "Usage: SAIL <island>"
expect missing_buy_args 'BUY\n' "Usage: BUY <product> <amount>"
expect missing_sell_arg 'SELL Barley\n' "Usage: SELL <product> <amount>"

echo "=== Extra argument ==="
expect extra_connect 'CONNECT ITHACA now\n' "Usage: CONNECT ITHACA"
expect extra_list_voyages 'LIST VOYAGES 2\n' "Usage: LIST VOYAGES"
expect extra_list_market 'LIST MARKET 2\n' "Usage: LIST MARKET"
expect extra_accept 'ACCEPT 2 3\n' "Usage: ACCEPT <voyage_id>"
expect extra_sail 'SAIL Aeaea now\n' "Usage: SAIL <island>"
expect extra_map 'MAP 1\n' "Usage: MAP"
expect extra_status 'STATUS now\n' "Usage: STATUS"
expect extra_deliver 'DELIVER x\n' "Usage: DELIVER"
expect extra_claim 'CLAIM x\n' "Usage: CLAIM"
expect extra_buy 'BUY Barley 2 x\n' "Usage: BUY <product> <amount>"
expect extra_sell 'SELL Barley 2 x\n' "Usage: SELL <product> <amount>"

echo "=== Prefix impostors ==="
expect impostor_maps 'MAPS\n' "Unknown command"
expect impostor_accepted 'ACCEPTED 2\n' "Unknown command"
expect impostor_buyer 'BUYER X 1\n' "Unknown command"

echo "=== Numeric junk, nonpositive, overflow ==="
expect junk_accept_alpha 'ACCEPT abc\n' "Usage: ACCEPT <voyage_id>"
expect junk_accept_mixed 'ACCEPT 2x\n' "Usage: ACCEPT <voyage_id>"
expect junk_buy_unit 'BUY Barley 3kg\n' "Usage: BUY <product> <amount>"
expect junk_sell_decimal 'SELL Barley 1.5\n' "Usage: SELL <product> <amount>"
expect zero_buy 'BUY Barley 0\n' "Usage: BUY <product> <amount>"
expect negative_sell 'SELL Barley -2\n' "Usage: SELL <product> <amount>"
expect accept_zero 'ACCEPT 0\n' "Usage: ACCEPT <voyage_id>"
expect accept_negative 'ACCEPT -1\n' "Usage: ACCEPT <voyage_id>"
expect accept_plus 'ACCEPT +2\n' "Usage: ACCEPT <voyage_id>"
expect accept_int_max 'ACCEPT 2147483647\n' "Command OK"
expect accept_int_max_plus_1 'ACCEPT 2147483648\n' "Usage: ACCEPT <voyage_id>"
expect accept_overflow 'ACCEPT 99999999999999999999\n' "Usage: ACCEPT <voyage_id>"
expect buy_overflow 'BUY Barley 99999999999999999999\n' "Usage: BUY <product> <amount>"
expect accept_leading_zeros 'ACCEPT 0002\n' "Command OK"
expect buy_leading_zeros 'BUY Barley 0003\n' "Command OK"

echo "=== State independence (syntax-only Phase 1) ==="
expect accept_absent_id 'ACCEPT 999\n' "Command OK"
expect sail_unknown_island 'SAIL Atlantis\n' "Command OK"
expect buy_unknown_product 'BUY UnknownProduct 3\n' "Command OK"

echo "=== Map special form (assumptions A06/A07) ==="
expect buy_map_1 'BUY MAP 1\n' "Command OK"
expect buy_map_2 'BUY MAP 2\n' "Usage: BUY <product> <amount>"
expect sell_map_1 'SELL MAP 1\n' "Command OK"

echo "=== Line handling ==="
expect blank_line '\n' ""
expect whitespace_only '   \t \n' ""
expect tabs_between_words 'BUY\tBarley\t3\n' "Command OK"
expect crlf_line 'MAP\r\nSTATUS\r\n' "Command OK
Command OK"
expect eof_without_newline 'MAP' "Command OK"
expect eof_partial_usage 'ACCEPT' "Usage: ACCEPT <voyage_id>"

echo "=== Process arguments and initialization failures ==="
# @Name: expect_exit
# @Def: Runs a command to completion and requires an exact exit status
#       and, for failures, a nonempty stderr diagnostic.
expect_exit() {
    name="$1"; want="$2"; shift 2
    "$@" </dev/null >"$WORK/out" 2>"$WORK/err"
    got=$?
    if [ "$got" != "$want" ]; then
        fail "$name" "expected exit $want, got $got"
    elif [ "$want" != "0" ] && [ ! -s "$WORK/err" ]; then
        fail "$name" "failure without stderr diagnostic"
    else
        pass
    fi
}
expect_exit odysseus_no_args 1 ./odysseus
expect_exit ithaca_no_args 1 ./ithaca configs/ithaca.dat
expect_exit island_no_args 1 ./island configs/aeaea.dat
expect_exit odysseus_missing_config 2 ./odysseus configs/does_not_exist.dat
expect_exit ithaca_missing_config 2 ./ithaca configs/does_not_exist.dat data/voyages.dat
expect_exit ithaca_missing_voyages 2 ./ithaca configs/ithaca.dat data/does_not_exist.dat
expect_exit island_missing_config 2 ./island configs/does_not_exist.dat data/stocks/Aeaea.db
expect_exit island_missing_stock 2 ./island configs/aeaea.dat data/stocks/does_not_exist.db
head -c 800 data/stocks/Aeaea.db >"$WORK/truncated.db"
expect_exit island_truncated_stock 2 ./island configs/aeaea.dat "$WORK/truncated.db"
head -n 4 configs/aeaea.dat >"$WORK/no_marker.dat"
expect_exit island_missing_marker 2 ./island "$WORK/no_marker.dat" data/stocks/Aeaea.db
head -n 6 configs/aeaea.dat >"$WORK/bad_second_route.dat"
echo "broken" >>"$WORK/bad_second_route.dat"
expect_exit island_malformed_later_route 2 ./island "$WORK/bad_second_route.dat" data/stocks/Aeaea.db
head -n 5 configs/odysseus.dat >"$WORK/short_odysseus.dat"
expect_exit odysseus_truncated_config 2 ./odysseus "$WORK/short_odysseus.dat"
printf 'Ship\nfolder\n127.0.0.1 99999\n' >"$WORK/bad_port.dat"
expect_exit odysseus_port_out_of_range 2 ./odysseus "$WORK/bad_port.dat"

echo "=== Long-lived processes: readiness and CTRL+C (SIGINT) ==="
# @Name: expect_sigint
# @Def: Starts a server-style process, waits (bounded) for its last
#       readiness line, checks it is still alive, sends SIGINT, waits
#       (bounded) for exit, and requires status 0, the shutdown line,
#       and empty stderr.
# @Arg: $1 = name, $2 = ready line, $3 = shutdown line, rest = command.
expect_sigint() {
    name="$1"; ready="$2"; bye="$3"; shift 3
    "$@" </dev/null >"$WORK/out" 2>"$WORK/err" &
    pid=$!
    tries=0
    while ! grep -qF "$ready" "$WORK/out" && [ "$tries" -lt 100 ]; do
        sleep 0.05; tries=$((tries + 1))
    done
    if ! kill -0 "$pid" 2>/dev/null; then
        wait "$pid"; fail "$name" "exited before SIGINT"; return
    fi
    kill -INT "$pid"
    tries=0
    while kill -0 "$pid" 2>/dev/null && [ "$tries" -lt 100 ]; do
        sleep 0.05; tries=$((tries + 1))
    done
    if kill -0 "$pid" 2>/dev/null; then
        kill -KILL "$pid"; wait "$pid"; fail "$name" "did not exit within 5 s of SIGINT"; return
    fi
    wait "$pid"
    status=$?
    if [ "$status" != "0" ] || ! grep -qxF "$bye" "$WORK/out" || [ -s "$WORK/err" ]; then
        fail "$name" "status=$status stdout=[$(tr '\n' '|' <"$WORK/out")] stderr=[$(cat "$WORK/err")]"
    else
        pass
    fi
}
VOYAGE_LINES=$(grep -c '[^[:space:]]' data/voyages.dat)
expect_sigint ithaca_sigint "Waiting for Odysseus..." "Ithaca closes the harbor." \
    ./ithaca configs/ithaca.dat data/voyages.dat
if grep -qxF "Ithaca initialized. $VOYAGE_LINES voyages loaded." "$WORK/out"; then pass; else fail ithaca_ready_count "$(head -n 1 "$WORK/out")"; fi
for isl in $ISLANDS; do
    lower=$(echo "$isl" | tr 'A-Z' 'a-z')
    expect_sigint "island_${lower}_sigint" "products available." "$isl closes its port." \
        ./island "configs/$lower.dat" "data/stocks/$isl.db"
done

# Odysseus: stdin kept open with a partial (unterminated) command, so
# only SIGINT can end the process.
mkfifo "$WORK/fifo"
./odysseus configs/odysseus.dat <"$WORK/fifo" >"$WORK/out" 2>"$WORK/err" &
pid=$!
exec 3>"$WORK/fifo"
printf 'MAP\nSTAT' >&3
tries=0
while ! grep -q "Command OK" "$WORK/out" && [ "$tries" -lt 100 ]; do sleep 0.05; tries=$((tries + 1)); done
kill -INT "$pid"
tries=0
while kill -0 "$pid" 2>/dev/null && [ "$tries" -lt 100 ]; do sleep 0.05; tries=$((tries + 1)); done
if kill -0 "$pid" 2>/dev/null; then kill -KILL "$pid"; fi
wait "$pid"
status=$?
exec 3>&-
if [ "$status" = "0" ] && [ "$(grep -c 'Command OK' "$WORK/out")" = "1" ] && [ ! -s "$WORK/err" ]; then
    pass
else
    fail odysseus_sigint_partial_line "status=$status stdout=[$(tr '\n' '|' <"$WORK/out")]"
fi

echo "=== Loader field-level comparisons (independent decode) ==="
# @Name: compare
# @Def: Diffs the loader_check output with independently built
#       expectations.
compare() {
    if diff "$WORK/expected" "$WORK/actual" >"$WORK/diff"; then
        pass
    else
        fail "$1" "$(head -n 6 "$WORK/diff" | tr '\n' '|')"
    fi
}

"$CHECKER" odysseus configs/odysseus.dat >"$WORK/actual" || fail odysseus_loader_status "loader_check failed"
tr -d '\r' <configs/odysseus.dat | awk '
    NR == 1 { print "odysseus.name=" $0 }
    NR == 2 { print "odysseus.storageFolder=" $0 }
    NR == 3 { print "odysseus.ithaca=" $1 ":" $2 }
    NR == 4 { print "odysseus.initialIsland=" $1 " " $2 ":" $3 }
    NR == 5 { print "odysseus.gold=" $1 }
    NR == 6 { print "odysseus.foodCount=" $1; n = $1 }
    NR > 6 && NR <= 6 + n { printf "odysseus.food[%d]=%s %s\n", NR - 7, $1, $2 }' >"$WORK/expected"
compare odysseus_fields

"$CHECKER" ithaca configs/ithaca.dat data/voyages.dat >"$WORK/actual" || fail ithaca_loader_status "loader_check failed"
{
    tr -d '\r' <configs/ithaca.dat | awk '
        NR == 1 { print "ithaca.name=" $0 }
        NR == 2 { print "ithaca.missionFolder=" $0 }
        NR == 3 { print "ithaca.listen=" $1 ":" $2 }'
    tr -d '\r' <data/voyages.dat | awk 'NF > 0 { n++; line[n] = $0 }
        END {
            print "ithaca.voyageCount=" n
            for (i = 1; i <= n; i++) {
                split(line[i], f, /[ \t]+/)
                printf "ithaca.voyage[%d]=id=%d object=%s file=%s dest=%s reward=%s\n", i - 1, i, f[1], f[2], f[3], f[4]
            }
        }'
} >"$WORK/expected"
compare ithaca_fields

# Survivor counts observed from the real Sphragis library and confirmed
# independently in PHASE1_AUDIT_RESULTS.md section 3 (test data only).
expected_survivors() {
    case "$1" in
        Aeaea) echo 2 ;; Aeolia) echo 1 ;; Ismarus) echo 1 ;;
        Ogygia) echo 1 ;; Scheria) echo 3 ;; Thrinacia) echo 2 ;;
        *) echo -1 ;;
    esac
}

for isl in $ISLANDS; do
    lower=$(echo "$isl" | tr 'A-Z' 'a-z')
    "$CHECKER" island "configs/$lower.dat" "data/stocks/$isl.db" >"$WORK/island_all" \
        || fail "${lower}_loader_status" "loader_check failed"
    # Header and raw routes, decoded from the text configuration.
    grep -v '^island\.\(sphragisResult\|validRoute\|product\)' "$WORK/island_all" >"$WORK/actual"
    tr -d '\r' <"configs/$lower.dat" | awk '
        NR == 1 { print "island.name=" $0 }
        NR == 2 { print "island.storageFolder=" $0 }
        NR == 3 { print "island.listen=" $1 ":" $2 }
        NR == 4 { print "island.capacity=" $1 }
        NR > 5 && NF > 0 { n++; r[n] = sprintf("island.rawRoute[%d]=%s %s:%s", n - 1, $1, $2, $3) }
        END { print "island.rawRouteCount=" n + 0; for (i = 1; i <= n; i++) print r[i] }' >"$WORK/expected"
    compare "${lower}_config_fields"
    # Every surviving route must keep the endpoint of its raw route, and
    # the count must equal both the library result and the observed one.
    RESULT=$(sed -n 's/^island\.sphragisResult=//p' "$WORK/island_all")
    VALID=$(sed -n 's/^island\.validRouteCount=//p' "$WORK/island_all")
    BAD=0
    for route in $(sed -n 's/^island\.validRoute\[[0-9]*\]=//p' "$WORK/island_all" | tr ' ' '#'); do
        if ! sed -n 's/^island\.rawRoute\[[0-9]*\]=//p' "$WORK/island_all" | tr ' ' '#' | grep -qxF "$route"; then
            BAD=1
        fi
    done
    if [ "$BAD" = "0" ] && [ "$RESULT" = "$VALID" ] && [ "$VALID" = "$(expected_survivors "$isl")" ]; then
        pass
    else
        fail "${lower}_filtered_routes" "result=$RESULT valid=$VALID endpointMismatch=$BAD"
    fi
    # Binary stock, decoded independently: 100 name bytes + 2 x int32 LE.
    grep '^island\.product' "$WORK/island_all" >"$WORK/actual"
    od -An -v -tu1 -w108 "data/stocks/$isl.db" | awk '
        function le(a, b, c, d,   v) { v = a + b * 256 + c * 65536 + d * 16777216; if (v >= 2147483648) v -= 4294967296; return v }
        BEGIN { n = 0 }
        NF == 108 {
            name = ""
            for (i = 1; i <= 100 && $i != 0; i++) name = name sprintf("%c", $i)
            r[n] = sprintf("island.product[%d]=%s amount=%d price=%d", n, name, le($101, $102, $103, $104), le($105, $106, $107, $108))
            n++
        }
        END { print "island.productCount=" n + 0; for (i = 0; i < n; i++) print r[i] }' >"$WORK/expected"
    compare "${lower}_stock_fields"
done

echo ""
echo "=== Summary: $PASS passed, $FAIL failed ==="
if [ "$FAIL" != "0" ]; then
    echo "Failed tests:$FAILED_NAMES"
    exit 1
fi
exit 0
