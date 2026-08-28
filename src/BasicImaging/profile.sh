#!/bin/bash
THISDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$THISDIR"

rm -f callgrind.out.*

# Rebuild the benchmark binary ( it lands in this directory ) so the profile always reflects the
# current sources. Requires the repo-level cmake build directory ( ./build/ at the repo root ).
cmake --build ../../build --target benchmarkresize || { echo "build failed - configure cmake from the repo root first"; exit 1; }

# Extra args ( --dir= --iterations= ) are forwarded to the benchmark ; the workload itself defaults
# to 1 iteration in --profile mode, which is what you want under valgrind's ~20x slowdown.
valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes ./benchmarkresize --profile "$@"

if command -v kcachegrind >/dev/null 2>&1; then
  kcachegrind
else
  echo "kcachegrind not found - callgrind_annotate callgrind.out.<pid> shows the same data as text"
fi
exit 0
