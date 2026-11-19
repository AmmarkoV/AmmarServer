#!/bin/bash
# Unified AmmarServer test runner.
#
#   ./tests/run_tests.sh                     # build + run the C unit tests and every test module
#   ./tests/run_tests.sh --no-build          # run without rebuilding
#   ./tests/run_tests.sh tests.test_security # run one module ( skips the C unit tests )
#
# The suite starts its own ammarserver instances on free loopback ports with
# temporary web roots, so it does not touch public_html/ or any running server.
# The C unit tests ( see C_UNIT_TESTS below ) log to build/tests/logs/<name>.log.

set -u

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
BUILD_DIR="$REPO_ROOT/build"
BINARY="${AMMARSERVER_BINARY:-$REPO_ROOT/src/Services/AmmarServer/ammarserver}"
LIBRARY_DIR="${AMMARSERVER_LIBRARY_DIR:-$REPO_ROOT/src/AmmServerlib}"

BUILD=1
if [ "${1:-}" = "--no-build" ]; then
  BUILD=0
  shift
fi

if [ "$BUILD" = "1" ]; then
  if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "build/ is not configured, running cmake first"
    cmake -S "$REPO_ROOT" -B "$BUILD_DIR" || exit 1
  fi
  echo "Building ammarserver and the library tests..."
  cmake --build "$BUILD_DIR" --target ammarserver testinputparser testhashmap testcsv FileToString StringEscapeFile testammcaptcha StringRecognizer testuseraccounts testammclient testbasicimaging -j"$(nproc)" || exit 1
  # testservice ( cookies , sessions , API mode ) is not a cmake target , see build_static.sh
  "$SCRIPT_DIR/build_static.sh" "$BUILD_DIR/tests" -O2 || exit 1
  # tests/unit/HashMapTester is only a cmake target with DO_TEST_COVERAGE , so build it here against the library
  gcc -O2 -w -o "$BUILD_DIR/tests/testHashMap" "$SCRIPT_DIR/unit/HashMapTester/testHashMap.c" -L"$LIBRARY_DIR" -lAmmarServer -lpthread || exit 1
fi

# name:binary , run from the repository root , exit code 0 = pass
C_UNIT_TESTS=(
  "hashmap-regressions:$REPO_ROOT/src/Hashmap/testhashmap"
  "csvparser-regressions:$REPO_ROOT/src/CSVParser/testcsv"
  "unit-hashmap:$BUILD_DIR/tests/testHashMap"
  "filetostring:$REPO_ROOT/src/FileToString/testFileToString.sh"
  "stringescapefile:$REPO_ROOT/src/StringEscapeFile/testStringEscapeFile.sh"
  "ammcaptcha:$REPO_ROOT/src/AmmCaptcha/testammcaptcha"
  "stringrecognizer:$REPO_ROOT/src/StringRecognizer/testStringRecognizer.sh"
  "useraccounts:$REPO_ROOT/src/UserAccounts/testuseraccounts"
  "ammclient:$REPO_ROOT/src/AmmClient/testammclient"
  "ammmessages:$REPO_ROOT/src/AmmMessages/testAmmMessages.sh"
  "basicimaging:$REPO_ROOT/src/BasicImaging/testbasicimaging"
)

run_c_unit_tests() {
  local logDir="$BUILD_DIR/tests/logs"
  local failed=0 entry name binary
  mkdir -p "$logDir"
  echo "Running C unit tests..."
  for entry in "${C_UNIT_TESTS[@]}"; do
    name="${entry%%:*}"
    binary="${entry#*:}"
    if [ ! -x "$binary" ]; then
      echo "  FAIL $name ( $binary not built )"
      failed=1
      continue
    fi
    if "$binary" > "$logDir/$name.log" 2>&1; then
      echo "  ok   $name"
    else
      echo "  FAIL $name ( exit $? , last lines of $logDir/$name.log below )"
      tail -n 15 "$logDir/$name.log" | sed 's/^/       /'
      failed=1
    fi
  done
  return $failed
}

if [ ! -x "$BINARY" ]; then
  echo "Server binary not found at $BINARY (build it first or set AMMARSERVER_BINARY)" >&2
  exit 1
fi

export LD_LIBRARY_PATH="$LIBRARY_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PYTHONPATH="$REPO_ROOT${PYTHONPATH:+:$PYTHONPATH}"

cd "$REPO_ROOT" || exit 1

if [ $# -gt 0 ]; then
  exec python3 -m unittest "$@" -v
fi

run_c_unit_tests
C_RESULT=$?

python3 -m unittest discover -s tests -t . -p 'test_*.py' -v
PY_RESULT=$?

if [ $C_RESULT -ne 0 ]; then echo "C unit tests FAILED ( see above )" >&2; fi
[ $C_RESULT -eq 0 ] && [ $PY_RESULT -eq 0 ]
