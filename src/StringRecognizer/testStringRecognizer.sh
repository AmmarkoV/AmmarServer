#!/bin/bash
# Regression tests for StringRecognizer ( the scanFor_* generator )
#   ./testStringRecognizer.sh [path/to/StringRecognizer]   ( default : the cmake build output next to this script )

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BIN="$( realpath "${1:-$SCRIPT_DIR/StringRecognizer}" )"
if [ ! -x "$BIN" ]; then echo "StringRecognizer binary not found at $BIN" >&2; exit 1; fi
SCANNERS="$SCRIPT_DIR/../AmmServerlib/stringscanners"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK" || exit 1
failures=0
check() { if eval "$2"; then :; else echo "FAIL : $1" >&2; failures=$((failures+1)); fi; }

# Failures used to exit 0 , so generateAmmServerScanners.sh carried on
"$BIN" > /dev/null 2>&1
check "no arguments fails" "[ $? -eq 1 ]"
"$BIN" no_such_list > /dev/null 2>&1
check "missing word list fails" "[ $? -eq 1 ]"

# Every committed scanner must still be what the generator produces from its word list ( ignoring the timestamp )
for list in httpHeader postHeader firstLines configKeys imageFiles textFiles audioFiles videoFiles applicationFiles archiveFiles; do
  cp "$SCRIPT_DIR/$list" .
  "$BIN" "$list" > /dev/null 2>&1
  for ext in c h; do
    check "$list.$ext regenerates identical to stringscanners/$list.$ext" \
          "diff -q <(grep -v 'automatically generated @' $list.$ext) <(grep -v 'automatically generated @' $SCANNERS/$list.$ext) > /dev/null"
  done
done

# A generated scanner recognises each of its words in any case , and nothing else
# ( 1 byte words used to make the generator read past their buffer , blank lines used to make an empty enum name )
printf 'A\nBC\n\nX-A:B\nZEBRA\nZERO\nZ9\n' > words
"$BIN" words > /dev/null 2>&1
check "word list with 1 byte words and a blank line generates" "[ $? -eq 0 ]"
check "mid-word ':' is dropped from the enum name ( was X_ABB )" "grep -q 'WORDS_X_AB,' words.h"
check "blank line does not produce an empty enum name" "! grep -q 'WORDS_,' words.h"
cat > driver.c <<'EOF'
#include <stdio.h>
#include <string.h>
#include "words.h"
static int expect(const char * s,int want) { int got=scanFor_words(s,strlen(s)); if (got!=want) { printf("scanFor_words(\"%s\") = %d , wanted %d\n",s,got,want); return 1; } return 0; }
int main()
{
  int bad=0;
  bad+=expect("A",WORDS_A);       bad+=expect("a",WORDS_A);
  bad+=expect("BC",WORDS_BC);     bad+=expect("bC",WORDS_BC);
  bad+=expect("X-A:B",WORDS_X_AB);
  bad+=expect("zebra",WORDS_ZEBRA); bad+=expect("ZERO",WORDS_ZERO); bad+=expect("z9",WORDS_Z9);
  bad+=expect("Q",0); bad+=expect("ZEB",0); bad+=expect("",0);
  return bad;
}
EOF
check "generated scanner compiles cleanly" "gcc -Wall -Werror -o driver driver.c words.c 2> driver.err"
check "generated scanner matches its words and rejects others" "./driver"

# More words than the 64 the generator holds used to overflow its table , it now refuses
seq -f "WORD%g" 1 70 > many
"$BIN" many > /dev/null 2>&1
check "more than 64 words is refused" "[ $? -eq 1 ]"

# The 1 byte word and overflow cases again under AddressSanitizer , when gcc can build it
if gcc -fsanitize=address -o asan_tool "$SCRIPT_DIR/main.c" "$SCRIPT_DIR/fastStringParser.c" 2> /dev/null; then
  ./asan_tool words > /dev/null 2> asan_words.err
  check "1 byte words are clean under AddressSanitizer" "[ $? -eq 0 ]"
  ./asan_tool many > /dev/null 2> asan_many.err
  check "too many words is clean under AddressSanitizer ( refused , exit 1 )" "[ $? -eq 1 ]"
fi

if [ $failures -ne 0 ]; then echo "StringRecognizer tests : $failures failures" >&2; exit 1; fi
echo "StringRecognizer tests : all passed" >&2
exit 0
