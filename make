#!/bin/bash

cd src/AmmServerlib
./make
cd ..

cd src/MyURL
./make
cd ..

./make
 
cd ..

if [ -e "src/ammarserver" ]; then
echo "Success creating ammarserver executable.." 
else
  echo "Error : Compilation of ammarserver executable failed..!" 
fi

exit 0