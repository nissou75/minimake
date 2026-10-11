#!/bin/sh

# Run every case in its own directory, preserving exact output and exit status.
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
BIN=${BIN:-"$PROJECT_ROOT/minimake"}
case "$BIN" in
    /*) ;;
    *) BIN="$(pwd)/$BIN" ;;
esac
if [ ! -x "$BIN" ]; then
    printf 'Functional tests: binary is not executable: %s\n' "$BIN" >&2
    exit 1
fi

SUITE_TMP=$(mktemp -d "${TMPDIR:-/tmp}/minimake-tests.XXXXXX") || exit 1
PASSED=0
FAILED=0
SKIPPED=0

cleanup()
{
    if [ "${KEEP_TMP:-0}" = 1 ]; then
        printf 'Test files retained in %s\n' "$SUITE_TMP"
    else
        rm -rf "$SUITE_TMP"
    fi
}

trap cleanup 0
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

fail()
{
    printf '  %s\n' "$*" >&2
    printf '  exit status: %s\n' "${STATUS:-not run}" >&2
    if [ -f "$OUT" ]; then
        printf '  stdout (escaped):\n' >&2
        sed -n '1,80l' "$OUT" >&2
    fi
    if [ -f "$ERR" ]; then
        printf '  stderr (escaped):\n' >&2
        sed -n '1,80l' "$ERR" >&2
    fi
    exit 1
}

run()
{
    "$BIN" "$@" >"$OUT" 2>"$ERR"
    STATUS=$?
}

expect_status()
{
    [ "$STATUS" -eq "$1" ] || fail "Expected exit status $1"
}

expect_stdout()
{
    cat >expected.stdout
    if ! cmp -s expected.stdout "$OUT"; then
        diff -u expected.stdout "$OUT" >&2
        fail 'Standard output differs'
    fi
}

expect_stderr()
{
    cat >expected.stderr
    if ! cmp -s expected.stderr "$ERR"; then
        diff -u expected.stderr "$ERR" >&2
        fail 'Standard error differs'
    fi
}

expect_empty_stdout()
{
    [ ! -s "$OUT" ] || fail 'Expected empty standard output'
}

expect_empty_stderr()
{
    [ ! -s "$ERR" ] || fail 'Expected empty standard error'
}

expect_error()
{
    expect_status 2
    [ -s "$ERR" ] || fail 'An error must be reported on standard error'
}

expect_success()
{
    expect_status 0
    expect_empty_stderr
}

expect_file()
{
    [ -f "$1" ] || fail "Expected file: $1"
}

expect_absent()
{
    [ ! -e "$1" ] || fail "Unexpected file: $1"
}

expect_content()
{
    cat >expected.content
    if ! cmp -s expected.content "$1"; then
        diff -u expected.content "$1" >&2
        fail "Content differs: $1"
    fi
}

run_case()
{
    case_name=$1
    case "$case_name" in
        ${TEST_FILTER:-*}) ;;
        *) return ;;
    esac
    mkdir "$SUITE_TMP/$case_name" || exit 1
    (
        cd "$SUITE_TMP/$case_name" || exit 1
        OUT="$PWD/stdout"
        ERR="$PWD/stderr"
        STATUS='not run'
        "$case_name"
    )
    case_status=$?
    case "$case_status" in
        0)
            PASSED=$((PASSED + 1))
            printf 'PASS %s\n' "$case_name"
            ;;
        77)
            SKIPPED=$((SKIPPED + 1))
            printf 'SKIP %s\n' "$case_name"
            ;;
        *)
            FAILED=$((FAILED + 1))
            printf 'FAIL %s\n' "$case_name"
            ;;
    esac
}

for test_file in "$PROJECT_ROOT"/tests/functional/*.sh; do
    . "$test_file"
done

printf '\nFunctional tests: %s passed, %s failed, %s skipped\n' \
    "$PASSED" "$FAILED" "$SKIPPED"
[ "$FAILED" -eq 0 ]
