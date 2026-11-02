#!/bin/bash

SERVER_TO_USE="127.0.0.1 8080"

red=$(printf "\033[31m")
green=$(printf "\033[32m") 
normal=$(printf "\033[m")


echo -e "GET / HTTP/1.0\n\n" | nc 127.0.0.1 8080 > resultSuccess
cat testfile1 | nc $SERVER_TO_USE > resultfile1
cat testfile2 | nc $SERVER_TO_USE > resultfile2
cat testfile3 | nc $SERVER_TO_USE > resultfile3
cat testfile4 | nc $SERVER_TO_USE > resultfile4
cat testfile5 | nc $SERVER_TO_USE > resultfile5
cat testfile6 | nc $SERVER_TO_USE > resultfile6
cat testfile7 | nc $SERVER_TO_USE > resultfile7
cat testfile8 | nc $SERVER_TO_USE > resultfile8
cat testfile9 | nc $SERVER_TO_USE > resultfile9
cat testfile10 | nc $SERVER_TO_USE > resultfile10
cat testfile11 | nc $SERVER_TO_USE > resultfile11


STILLOK=`ps -A | grep ammarserver`

if [ -z "$STILLOK" ]
then
 echo "$red FAILED :( , time for some coding to fix stuff.. $normal"
else
 echo "$green We passed pentest check $normal"
fi

exit 0
