#!/bin/bash


for item in httpHeader postHeader firstLines imageFiles textFiles audioFiles videoFiles applicationFiles ; do
    echo "Generating $item header"
    ./StringRecognizer $item
    #gcc "$item.c" -o "$item-Scanner"

    cp "$item.c" ../AmmServerlib/stringscanners/
    cp "$item.h" ../AmmServerlib/stringscanners/
done

 
exit 0
