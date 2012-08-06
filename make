#!/bin/bash

cd src/AmmServerlib
./make
cd ..
./make

# NOT READY YET..!
#cd WebCamHttpBridge
#./make
#cd VideoInput
#./make
#cd ..
#cd ..

cd ..


if [ -e "src/ammarserver" ]; then
echo "Success creating ammarserver executable.." 
else
  echo "Error : Compilation of ammarserver executable failed..!" 
fi

exit 0