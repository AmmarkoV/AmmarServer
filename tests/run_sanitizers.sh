#!/bin/bash
# Run the automated test suite against binaries built with AddressSanitizer,
# LeakSanitizer and UndefinedBehaviorSanitizer.
#
#   ./tests/run_sanitizers.sh                      # every test module
#   ./tests/run_sanitizers.sh tests.test_security  # only some modules
#
# Catches what valgrind's memcheck cannot (stack and global buffer overflows,
# undefined behaviour) and is several times faster. Binaries are built into
# build/sanitizers/ (see build_static.sh); every report a server process
# writes lands in build/sanitizers/logs/.
#
# Exit status: 0 when the tests pass and no report was written, 1 otherwise.

set -u

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
OUT_DIR="$REPO_ROOT/build/sanitizers"
LOG_DIR="$OUT_DIR/logs"

cd "$REPO_ROOT" || exit 1
rm -rf "$LOG_DIR"
mkdir -p "$LOG_DIR"

echo "Building sanitizer binaries into $OUT_DIR ..."
"$SCRIPT_DIR/build_static.sh" "$OUT_DIR" -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined || exit 1

export AMMARSERVER_BINARY="$OUT_DIR/ammarserver"
export AMMARSERVER_TESTSERVICE_BINARY="$OUT_DIR/testservice"
export ASAN_OPTIONS="log_path=$LOG_DIR/asan:detect_leaks=1"
export UBSAN_OPTIONS="log_path=$LOG_DIR/ubsan:print_stacktrace=1"
export PYTHONPATH="$REPO_ROOT${PYTHONPATH:+:$PYTHONPATH}"

if [ $# -gt 0 ]; then
  python3 -m unittest "$@" -v
else
  python3 -m unittest discover -s tests -t . -p 'test_*.py' -v
fi
TEST_STATUS=$?

echo
echo "=== sanitizer summary ($LOG_DIR) ==="
REPORTS="$(ls "$LOG_DIR" | wc -l)"
grep -hE 'ERROR: (Address|Leak)Sanitizer|runtime error:' "$LOG_DIR"/* 2>/dev/null \
  | sed -E 's/^==[0-9]+==//; s/0x[0-9a-f]+/ADDR/g' | sort | uniq -c | sort -rn
echo "$REPORTS report file(s), test exit status $TEST_STATUS"

[ "$REPORTS" -eq 0 ] && [ "$TEST_STATUS" -eq 0 ]
