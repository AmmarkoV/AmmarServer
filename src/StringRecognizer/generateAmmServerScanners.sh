#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

mkdir binaries

for item in httpHeader postHeader firstLines imageFiles textFiles audioFiles videoFiles applicationFiles archiveFiles; do
    echo "Generating $item header"
    ./StringRecognizer $item
    gcc -S "$item.c" -o "binaries/$item-Scanner"

    cp "$item.c" ../AmmServerlib/stringscanners/
    cp "$item.h" ../AmmServerlib/stringscanners/

    rm "$item.c" "$item.h"
done

 
exit 0
