#!/bin/bash

DIFF_S=1

usage()
{
    cat << EOF
usage: $(basename "$0") [options] TEST_DIR TEST_PROG [ARGUMENT...]

Run integration tests

Options:
  -s    Don't diff assembly files even if available, default false.
  -h    Print this message
EOF
}

while getopts "sh" Option
do
    case $Option in
        s)
            DIFF_S=
            ;;
        h)
            usage
            exit 0
            ;;
        ?)
            usage
            exit 85
            ;;
    esac
done

shift $((OPTIND-1))
if [[ $# -lt 2 ]]; then
    >&2 echo "Error: Missing positional arguments"
    >&2 usage
    exit 1
fi

TEST_DIR="$1"
shift 1

# Create temporary directory for output files, accounting for differences
# between Linux and OSX
OUT_DIR=$(mktemp -d 2> /dev/null || mktemp -d -t 'tmp') || exit 1
trap "rm -rf $OUT_DIR" 0

cd "$TEST_DIR"

# Find all tests
tests=( $(find "." -name "*.test") )

# Track number of passing tests
total=0
passed=0

for test in "${tests[@]}"; do
    (( ++total ))

    name=$(basename "$test")
    echo "Testing ${name}"

    $* -w "$OUT_DIR" "$test" > "${OUT_DIR}/${name}.stdout" 2> "${OUT_DIR}/${name}.stderr"

    test_exit=0

    # If we have a assembly file to compare against, do so...
    if [[ $DIFF_S &&  -e "${test}.s" ]]; then
        diff -b -w "${test}.s" "${OUT_DIR}/${name}.s"
        if [[ $? -ne 0 ]]; then
            test_exit=1
        fi
    fi

    # Compare against known stdout and stderr
    diff -b -w "${test}.stdout" "${OUT_DIR}/${name}.stdout"
    if [[ $? -ne 0 ]]; then
        test_exit=1
    fi

    # Ignore different orders in the error messages
    diff -b -w <(sort "${test}.stderr") <(sort "${OUT_DIR}/${name}.stderr")
    if [[ $? -ne 0 ]]; then
        test_exit=1
    fi

    if [[ $test_exit -eq 0 ]]; then
        (( ++passed ))
    fi
done

echo "Total tests: $total"
echo "Passing tests: $passed"
echo "Failing tests: $((total - passed))"

if [[ $passed -ne $total ]]; then
    exit 1
else
    exit 0
fi