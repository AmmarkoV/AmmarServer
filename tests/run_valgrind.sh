#!/bin/bash
# Run the automated test suite against a server running under valgrind.
#
#   ./tests/run_valgrind.sh                      # every test module, one after the other
#   ./tests/run_valgrind.sh tests.test_security  # only some modules
#   ./tests/run_valgrind.sh --helgrind [modules] # data races / lock order instead of memory errors
#
# Dedicated debug binaries (-g -O1, not stripped, everything linked statically
# into one executable, see build_static.sh) are built into build/valgrind/ so
# the regular cmake build is left untouched. Every server instance the suite starts is wrapped in
# valgrind memcheck; one report per server process lands in
# build/valgrind/logs/ and a summary is printed at the end.
#
# Exit status: 0 when the tests pass and no report contains an error, a leak
# or a descriptor left open at exit, 1 otherwise. Known-benign reports are
# listed in tests/valgrind.supp.

set -u

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
OUT_DIR="$REPO_ROOT/build/valgrind"
LOG_DIR="$OUT_DIR/logs"
BINARY="$OUT_DIR/ammarserver"

export AMMARSERVER_VALGRIND_TOOL=memcheck
if [ "${1:-}" = "--helgrind" ]; then
  AMMARSERVER_VALGRIND_TOOL=helgrind
  LOG_DIR="$OUT_DIR/helgrind-logs"
  shift
fi

command -v valgrind >/dev/null || { echo "valgrind is not installed" >&2; exit 1; }

cd "$REPO_ROOT" || exit 1
mkdir -p "$OUT_DIR"
rm -rf "$LOG_DIR"
mkdir -p "$LOG_DIR"

echo "Building debug binaries into $OUT_DIR ..."
"$SCRIPT_DIR/build_static.sh" "$OUT_DIR" -g -O1 -fno-omit-frame-pointer || exit 1

export AMMARSERVER_BINARY="$BINARY"
export AMMARSERVER_TESTSERVICE_BINARY="$OUT_DIR/testservice"
export AMMARSERVER_VALGRIND_LOG_DIR="$LOG_DIR"
export PYTHONPATH="$REPO_ROOT${PYTHONPATH:+:$PYTHONPATH}"

if [ $# -eq 0 ]; then
  MODULES=""
  for file in tests/test_*.py; do
    MODULES="$MODULES tests.$(basename "$file" .py)"
  done
  # test_inputparser does not start a server, nothing for valgrind to see there
  MODULES="${MODULES/ tests.test_inputparser/}"
else
  MODULES="$*"
fi

TEST_FAILURES=0
for module in $MODULES; do
  echo
  echo "=== $module (under valgrind) ==="
  python3 -m unittest "$module" -v || TEST_FAILURES=$((TEST_FAILURES+1))
done

echo
echo "=== valgrind summary ($LOG_DIR) ==="
DIRTY=0
for log in "$LOG_DIR"/valgrind.*.log; do
  [ -f "$log" ] || continue
  errors="$(grep -E 'ERROR SUMMARY: [1-9]' "$log" | sed 's/^==[0-9]*== //')"
  leaks="$(grep -E '(definitely|indirectly) lost: [1-9]' "$log" | sed 's/^==[0-9]*== *//' | tr '\n' ' ')"
  fds="$(grep -E 'FILE DESCRIPTORS: ' "$log" | sed 's/^==[0-9]*== //')"
  # 3 std descriptors + the valgrind report itself are expected , anything else was left open
  open_fds="$(echo "$fds" | sed -E 's/.*: ([0-9]+) open.*/\1/')"
  [ -n "$fds" ] || open_fds=0   # helgrind does not track descriptors
  if [ -n "$errors" ] || [ -n "$leaks" ] || [ "${open_fds:-0}" -gt 4 ]; then
    DIRTY=$((DIRTY+1))
    echo "$(basename "$log"): ${errors:-no errors} ${leaks:+| $leaks}| $fds"
  fi
  # A report without a summary means the server died or had to be killed
  if ! grep -q 'ERROR SUMMARY' "$log"; then
    DIRTY=$((DIRTY+1))
    echo "$(basename "$log"): no ERROR SUMMARY (server crashed or was killed)"
  fi
done

TOTAL="$(ls "$LOG_DIR" | wc -l)"
echo "$DIRTY of $TOTAL server runs have valgrind findings, $TEST_FAILURES test module(s) failed"

[ "$DIRTY" -eq 0 ] && [ "$TEST_FAILURES" -eq 0 ]
