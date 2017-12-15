#!/bin/bash

#This is the script started by the startup script so we might want to customize it here..

STARTDIR=`pwd`
#Switch to this directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

./startAmmarServerSuite.sh


exit 0

