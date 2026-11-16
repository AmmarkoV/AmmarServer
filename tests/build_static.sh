#!/bin/bash
# Build the two binaries the test suite drives as self-contained executables
# (library, Hashmap and InputParser compiled in, nothing taken from the cmake
# build):
#
#   ./tests/build_static.sh OUT_DIR [extra gcc flags...]
#
#   OUT_DIR/ammarserver    src/Services/AmmarServer
#   OUT_DIR/testservice    tests/fixtures/testservice.c (cookies, sessions, API mode)
#
# Used by run_tests.sh (testservice only matters there), run_valgrind.sh and
# run_sanitizers.sh, each with its own flags.

set -eu

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
OUT_DIR="$1"
shift

cd "$REPO_ROOT"
mkdir -p "$OUT_DIR"

mapfile -t LIBRARY < <(find src/AmmServerlib -name '*.c' | sort)
LIBRARY+=(src/Hashmap/hashmap.c src/InputParser/InputParser_C.c)

gcc -w "$@" -o "$OUT_DIR/ammarserver" "${LIBRARY[@]}" src/Services/AmmarServer/main.c -lpthread -lrt &
gcc -w "$@" -o "$OUT_DIR/testservice" "${LIBRARY[@]}" tests/fixtures/testservice.c -lpthread -lrt &
wait %1
wait %2
