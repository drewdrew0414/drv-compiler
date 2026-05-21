#!/usr/bin/env bash
# dri regression test runner
# Usage: ./tests/run_tests.sh [--dri <path>] [--update] [filter]
#   --dri <path>   override compiler binary (default: ./cmake-build-debug/dri)
#   --update       regenerate expected outputs instead of comparing
#   filter         only run tests matching this substring

cd "$(dirname "$0")/.."

DRI="./cmake-build-debug/dri"
UPDATE=0
FILTER=""

while [ $# -gt 0 ]; do
    case "$1" in
        --dri)    DRI="$2"; shift 2 ;;
        --update) UPDATE=1; shift ;;
        *)        FILTER="$1"; shift ;;
    esac
done

if [ ! -x "$DRI" ]; then
    echo "error: dri binary not found at $DRI" >&2
    exit 1
fi

PASS=0; FAIL=0; SKIP=0
FAILED_LIST=""

# Strip floating-point timing values from lines for fuzzy comparison.
# Patterns: numbers with decimal points (e.g. 1234.5 ms, 0.884 GB/s)
strip_timing() {
    sed 's/[0-9][0-9]*\.[0-9][0-9]*/NUM/g'
}

run_one() {
    local src="$1"
    local name
    name=$(basename "$src" .dri)
    local expected="tests/cases/${name}.expected"

    if [ -n "$FILTER" ]; then
        case "$name" in *"$FILTER"*) ;; *) SKIP=$((SKIP+1)); return ;; esac
    fi

    local actual exit_code
    actual=$("$DRI" "$src" 2>/dev/null); exit_code=$?

    if [ $UPDATE -eq 1 ]; then
        printf '%s\n' "$actual" > "$expected"
        echo "  updated: $name"
        return
    fi

    if [ ! -f "$expected" ]; then
        echo "  SKIP  (no .expected): $name"
        SKIP=$((SKIP+1))
        return
    fi

    local expected_content
    expected_content=$(cat "$expected")

    if [ "$actual" = "$expected_content" ] && [ $exit_code -eq 0 ]; then
        echo "  PASS: $name"
        PASS=$((PASS+1))
    else
        echo "  FAIL: $name"
        if [ $exit_code -ne 0 ]; then
            echo "    exit: $exit_code"
            "$DRI" "$src" 2>&1 | grep -v "^dri: warning\|macOS:\|Linux:" | head -5 | sed 's/^/    /' || true
        else
            diff <(printf '%s\n' "$expected_content") <(printf '%s\n' "$actual") | head -8 | sed 's/^/    /' || true
        fi
        FAIL=$((FAIL+1))
        FAILED_LIST="$FAILED_LIST $name"
    fi
}

run_example() {
    local src="$1"
    local name
    name=$(basename "$src" .dri)
    local expected="tests/cases/${name}.expected"

    if [ -n "$FILTER" ]; then
        case "$name" in *"$FILTER"*) ;; *) SKIP=$((SKIP+1)); return ;; esac
    fi

    "$DRI" "$src" > /tmp/dri_test_actual.txt 2>/dev/null
    local exit_code=$?

    if [ $UPDATE -eq 1 ]; then
        cp /tmp/dri_test_actual.txt "$expected"
        echo "  updated: $name"
        return
    fi

    if [ $exit_code -ne 0 ]; then
        echo "  FAIL: $name  (exit=$exit_code)"
        FAIL=$((FAIL+1))
        FAILED_LIST="$FAILED_LIST $name"
        return
    fi

    if [ ! -f "$expected" ]; then
        # No expected file: just check it runs without error
        echo "  PASS: $name  (exit-code only)"
        PASS=$((PASS+1))
        return
    fi

    # Fuzzy compare: strip floating-point numbers for timing-sensitive benchmarks
    local norm_expected norm_actual
    norm_expected=$(strip_timing < "$expected")
    norm_actual=$(strip_timing < /tmp/dri_test_actual.txt)

    if [ "$norm_expected" = "$norm_actual" ]; then
        echo "  PASS: $name"
        PASS=$((PASS+1))
    else
        echo "  FAIL: $name"
        diff <(echo "$norm_expected") <(echo "$norm_actual") | head -8 | sed 's/^/    /' || true
        FAIL=$((FAIL+1))
        FAILED_LIST="$FAILED_LIST $name"
    fi
}

echo "dri regression tests"
echo "  compiler : $DRI"
echo "  mode     : $([ $UPDATE -eq 1 ] && echo 'update' || echo 'compare')"
echo ""

echo "── unit tests ──────────────────────────────────────────────"
for f in tests/cases/*.dri; do
    run_one "$f"
done

echo ""
echo "── example tests ───────────────────────────────────────────"
for f in docs/ko/example/example*.dri; do
    run_example "$f"
done

echo ""
echo "────────────────────────────────────────────────────────────"
echo "  PASS: $PASS   FAIL: $FAIL   SKIP: $SKIP"
if [ -n "$FAILED_LIST" ]; then
    echo ""
    echo "Failed:"
    for t in $FAILED_LIST; do echo "  - $t"; done
fi
echo ""

[ $FAIL -eq 0 ]
