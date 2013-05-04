#!/bin/bash

#echo "Hand made make script for InputParser - InputParser_C_Tester .."
#echo "Should provide libInputParser.a and InputParser_C_Tester/bin/Release/InputParserC_Tester"
#echo "Written by Ammar Qammaz a.k.a AmmarkoV :) "
#echo "Compiling files.." 
#gcc -c InputParser.cpp -o InputParser.o
#gcc -c InputParser_C.c -o InputParser_C.o 

#echo "Linking files.."
#ar  rcs libInputParser.a InputParser.o
#ar  rcs libInputParser_C.a InputParser_C.o

#echo "Cleaning compiled object files.."
#rm InputParser.o InputParser_C.o 

make

echo "Compiling Tester.."
cd InputParser_C_Tester
./make
cd ..

echo "Done.."
exit 0
