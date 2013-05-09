#!/bin/bash

cd src/AmmServerlib
./make
cd ..


cd AmmServerNULLlib
./make
cd ..
 
cd SimpleTemplate
./make
cd ..

cd MyURL
./make
cd ..
 
cd MyLoader
./make
cd ..

cd ScriptRunner
./make
cd ..

./make
 
cd ..

if [ -e "src/ammarserver" ]; then
  echo "Success creating ammarserver executable.." 
  strip src/ammarserver
else
  echo "Error : Compilation of ammarserver executable failed..!" 
fi

exit 0
