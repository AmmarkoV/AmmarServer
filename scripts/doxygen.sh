#!/bin/bash

#Switch to this directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"
 
export AMMSRV_DOXYGEN_INPUT="$DIR/.."
export AMMSRV_DOXYGEN_OUTPUT="$DIR/../doc"

cd ..
doxygen doc/doxyfile
cd doc/latex
make
cd .. 
cd ..

ln -s doc/latex/refman.pdf AmmarServer.pdf


cd "$STARTDIR"
 

exit 0
