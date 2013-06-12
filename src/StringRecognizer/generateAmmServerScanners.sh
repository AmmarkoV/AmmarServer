#!/bin/bash

./StringRecognizer httpHeader
gcc httpHeader.c -o httpHeaderScanner

cp httpHeader.c ../AmmServerlib/stringscanners/
cp httpHeader.h ../AmmServerlib/stringscanners/



./StringRecognizer postHeader
gcc postHeader.c -o postHeaderScanner

cp postHeader.c ../AmmServerlib/stringscanners/
cp postHeader.h ../AmmServerlib/stringscanners/

exit 0
