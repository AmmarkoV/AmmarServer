#!/bin/bash
gcc testHashmap.c hashmap.c -o testHashmap -lpthread && ./testHashmap
exit $?
