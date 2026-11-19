#!/bin/bash

DIRTOUNITTESTS="../unit/HashMapTester/"

if [ -d $DIRTOUNITTESTS ]
then
 echo "UnitTest dir is $DIRTOUNITTESTS"
else
 DIRTOUNITTESTS="tests/unit/HashMapTester/"
 if [ -d $DIRTOUNITTESTS ]
  then
   echo "UnitTest dir is $DIRTOUNITTESTS"
 fi
fi 

#time "$DIRTOUNITTESTS"testHashMap

valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes  --track-origins=yes "$DIRTOUNITTESTS"testHashMap 2> testHashMap.log&

exit 0
