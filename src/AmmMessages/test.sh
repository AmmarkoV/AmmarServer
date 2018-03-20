#!/bin/bash

cd output 
ln -s ../mmapBridge.h .
ln -s ../mmapBridge.c .
cd ..

./AmmMessages -msg samples/msg/person.msg person
astyle person.h 
mv person.h output/
gcc output/person.h -o output/person

exit 0
