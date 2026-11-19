#!/bin/bash
# Regression tests for StringEscapeFile
#   ./testStringEscapeFile.sh [path/to/StringEscapeFile]   ( default : the cmake build output next to this script )

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BIN="$( realpath "${1:-$SCRIPT_DIR/StringEscapeFile}" )"
if [ ! -x "$BIN" ]; then echo "StringEscapeFile binary not found at $BIN" >&2; exit 1; fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK" || exit 1
failures=0
check() { if eval "$2"; then :; else echo "FAIL : $1" >&2; failures=$((failures+1)); fi; }

# Compiles the generated literal and prints it the way the server uses it ( as a printf format )
decode() {
  printf '#include <stdio.h>\nint main(){printf(\n#include "%s"\n);return 0;}\n' "$1" > decode.c
  gcc -Werror -Wall -Wno-format-security decode.c -o decode 2> decode.err && ./decode
}

# Exact output format ( recreateFiles.sh depends on it ) , lines are joined by \<newline> continuations
printf '<p>a</p>\n<p>b</p>\n' > simple.html
"$BIN" simple.html simple.out > /dev/null
check "simple file exit code" "[ $? -eq 0 ]"
check "simple file output" "[ \"\$(cat simple.out)\" = \"\$(printf '\"<p>a</p>\\\\\n<p>b</p>\\\\\n\"')\" ]"

# Quotes , percent signs , backslashes , blank lines and CRLF all survive a compile + printf round trip
printf 'say "hi" 100%%\r\n\nback\\slash <\\/title> a\\nb\n\n' > mixed.html
"$BIN" mixed.html mixed.out > /dev/null
check "mixed file exit code" "[ $? -eq 0 ]"
check "mixed file literal compiles without warnings ( \\\\/ used to warn )" "decode mixed.out > decoded.txt"
check "mixed file decodes to the original text" "[ \"\$(cat decoded.txt)\" = 'say \"hi\" 100%back\\slash <\\/title> a\\nb' ]"

# Blank lines used to read line[-1] , run the case under AddressSanitizer when gcc can build it
if gcc -fsanitize=address -o asan_tool "$SCRIPT_DIR/stringescapefile.c" 2> /dev/null; then
  printf 'a\n\n\nb\n' > blank.html
  ASAN_OPTIONS=detect_leaks=1 ./asan_tool blank.html blank.out > /dev/null 2> asan.err
  check "blank lines are clean under AddressSanitizer ( see asan.err )" "[ $? -eq 0 ]"
fi

# A missing input used to truncate an existing output before failing
echo "precious" > existing.out
"$BIN" no_such_input.html existing.out > /dev/null 2>&1
check "missing input fails" "[ $? -eq 1 ]"
check "missing input leaves an existing output untouched" "[ \"\$(cat existing.out)\" = precious ]"

# A directory input fails cleanly and leaves no output
mkdir adir
timeout 5 "$BIN" adir dir.out > /dev/null 2>&1
check "directory input fails ( 124 = hung )" "[ $? -eq 1 ]"
check "directory input leaves no output" "[ ! -e dir.out ]"

# Unwritable output and missing arguments
"$BIN" simple.html /nonexistent/dir/out > /dev/null 2>&1
check "unwritable output fails" "[ $? -eq 1 ]"
"$BIN" simple.html > /dev/null 2>&1
check "missing argument fails" "[ $? -eq 1 ]"

if [ $failures -ne 0 ]; then echo "StringEscapeFile tests : $failures failures" >&2; exit 1; fi
echo "StringEscapeFile tests : all passed" >&2
exit 0
