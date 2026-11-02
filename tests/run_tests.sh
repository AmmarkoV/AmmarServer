#!/bin/bash
# Unified AmmarServer test runner.
#
#   ./tests/run_tests.sh                     # build + run every test module
#   ./tests/run_tests.sh --no-build          # run without rebuilding
#   ./tests/run_tests.sh tests.test_security # run one module
#
# The suite starts its own ammarserver instances on free loopback ports with
# temporary web roots, so it does not touch public_html/ or any running server.

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
  echo "Building ammarserver and InputParser tests..."
  cmake --build "$BUILD_DIR" --target ammarserver testinputparser -j"$(nproc)" || exit 1
fi

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

exec python3 -m unittest discover -s tests -t . -p 'test_*.py' -v
