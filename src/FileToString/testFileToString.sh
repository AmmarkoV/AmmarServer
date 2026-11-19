#!/bin/bash
# Regression tests for FileToString
#   ./testFileToString.sh [path/to/FileToString]   ( default : the cmake build output next to this script )

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BIN="$( realpath "${1:-$SCRIPT_DIR/FileToString}" )"
if [ ! -x "$BIN" ]; then echo "FileToString binary not found at $BIN" >&2; exit 1; fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK" || exit 1
failures=0
check() { if eval "$2"; then :; else echo "FAIL : $1" >&2; failures=$((failures+1)); fi; }

# A normal file : exact output format ( recreateFiles.sh depends on it )
printf 'AB' > icon.gif
"$BIN" icon.gif out.h > /dev/null
check "normal file exit code" "[ $? -eq 0 ]"
check "normal file output" "[ \"\$(cat out.h)\" = \"\$(printf '#pragma once\n static unsigned char icon_gif[] = { 0x41, 0x42, };')\" ]"

# A missing input used to dereference a null FILE* and segfault
"$BIN" no_such_file.gif missing.h > /dev/null 2>&1
check "missing input fails cleanly ( not a crash )" "[ $? -eq 1 ]"
check "missing input leaves no output file" "[ ! -e missing.h ]"

# A directory input used to loop forever , fread() fails but feof() is never set
mkdir adir
timeout 5 "$BIN" adir dir.h > /dev/null 2>&1
check "directory input terminates and fails ( exit 1 , 124 = hung )" "[ $? -eq 1 ]"
check "directory input leaves no truncated output" "[ ! -e dir.h ]"

# Names that are not valid C identifiers used to produce headers that don't compile
printf 'xyz' > my-icon.v2.gif
"$BIN" my-icon.v2.gif dash.h > /dev/null
check "hyphenated name exit code" "[ $? -eq 0 ]"
check "hyphenated name becomes an identifier" "grep -q 'my_icon_v2_gif\[\]' dash.h"
check "hyphenated name header compiles" "gcc -fsyntax-only -x c dash.h 2>/dev/null"

printf 'z' > 9lives.gif
"$BIN" 9lives.gif digit.h > /dev/null
check "leading digit gets a _ prefix" "grep -q '_9lives_gif\[\]' digit.h"
check "leading digit header compiles" "gcc -fsyntax-only -x c digit.h 2>/dev/null"

# Unwritable output
"$BIN" icon.gif /nonexistent/dir/out.h > /dev/null 2>&1
check "unwritable output fails" "[ $? -eq 1 ]"

# Fewer arguments than needed
"$BIN" icon.gif > /dev/null 2>&1
check "missing argument fails" "[ $? -eq 1 ]"

if [ $failures -ne 0 ]; then echo "FileToString tests : $failures failures" >&2; exit 1; fi
echo "FileToString tests : all passed" >&2
exit 0
