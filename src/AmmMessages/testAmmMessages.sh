#!/bin/bash
# Regression tests for AmmMessages ( the .msg -> C header generator and its mmap bridge )
#   ./testAmmMessages.sh [path/to/AmmMessages]
# Without an argument it uses the cmake build output next to this script , or , since that target only exists with
# USE_AMMMESSAGES ( and its post-build step writes into the source tree ) , builds a private copy with gcc.

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SRC="$( cd "$SCRIPT_DIR/.." && pwd )"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK" || exit 1

if [ -n "${1:-}" ]; then BIN="$( realpath "$1" )"; else BIN="$SCRIPT_DIR/AmmMessages"; fi
if [ -z "${1:-}" ] && [ ! -x "$BIN" ]; then
  BIN="$WORK/AmmMessages"
  gcc -O2 -w -o "$BIN" "$SCRIPT_DIR/main.c" "$SCRIPT_DIR/messageGenerator.c" "$SCRIPT_DIR/centralIncludeGenerator.c" \
      "$SRC/StringRecognizer/fastStringParser.c" "$SRC/InputParser/InputParser_C.c" -lm || { echo "Could not build AmmMessages" >&2; exit 1; }
fi
if [ ! -x "$BIN" ]; then echo "AmmMessages binary not found at $BIN" >&2; exit 1; fi
failures=0
check() { if eval "$2"; then :; else echo "FAIL : $1" >&2; failures=$((failures+1)); fi; }

# Failures used to exit 0 , -msg read missing arguments , and a missing .msg file was dereferenced
"$BIN" > /dev/null 2>&1;                                   check "no arguments fails" "[ $? -eq 1 ]"
"$BIN" -msg > /dev/null 2>&1;                              check "-msg without its arguments fails" "[ $? -eq 1 ]"
"$BIN" -msg missing.msg missing "$WORK" > /dev/null 2>&1;  check "missing .msg file fails" "[ $? -eq 1 ]"
"$BIN" -msg "$SCRIPT_DIR/samples/msg/person.msg" my-person "$WORK" > /dev/null 2>&1
check "a name that is not a C identifier is refused" "[ $? -eq 1 ] && [ ! -e my-person.h ]"

# Every sample generates , and compiles with all its optional sections switched on
# ( the /flush/ callback used to have no return statement )
printf '#include <stdlib.h>\n#include "InputParser_C.h"\n#include "AmmServerlib.h"\n#include "AmmClient.h"\n#include MSGHEADER\nint main(){return 0;}\n' > compile.c
for msg in "$SCRIPT_DIR"/samples/msg/*.msg; do
  name="$(basename "$msg" .msg)"
  "$BIN" -msg "$msg" "$name" "$WORK" > /dev/null 2>&1
  check "$name.msg generates" "[ $? -eq 0 ] && [ -s $name.h ]"
  check "$name.h compiles with -Werror=return-type -Werror=format" \
        "gcc -fsyntax-only -Wall -Werror=return-type -Werror=format -Wno-unused-function -I. -I$SCRIPT_DIR -I$SRC/InputParser -I$SRC/AmmServerlib -I$SRC/AmmClient -DMSGHEADER='\"$name.h\"' compile.c 2> $name.err"
  check "$name.h URL has no field without a value" "! grep -q 'return snprintf.*=[A-Za-z]' $name.h"
done

# Round trip ( negative values , '=' in a response header , unpack into the given message ) and the mmap bridge
check "round trip program builds" \
      "gcc -Wall -I. -I$SCRIPT_DIR -I$SRC/InputParser -o roundtrip $SCRIPT_DIR/testRoundTrip.c $SCRIPT_DIR/mmapBridge.c $SRC/InputParser/InputParser_C.c -lm 2> roundtrip.err"
if [ -x roundtrip ]; then
  ./roundtrip "$WORK" > roundtrip.out 2>&1
  check "round trip and bridge ( see roundtrip.out )" "[ $? -eq 0 ]"
  grep "FAIL" roundtrip.out >&2
fi

if [ $failures -ne 0 ]; then echo "AmmMessages tests : $failures failures" >&2; exit 1; fi
echo "AmmMessages tests : all passed" >&2
exit 0
