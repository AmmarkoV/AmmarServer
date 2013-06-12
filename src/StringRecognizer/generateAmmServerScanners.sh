#!/bin/bash

./StringRecognizer httpHeader
gcc httpHeader.c -o httpHeaderScanner

cp httpHeader.c ../AmmServerlib/stringscanners/
cp httpHeader.h ../AmmServerlib/stringscanners/

exit 0
