#!/bin/bash

STARTDIR=`pwd`
#Switch to this directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

echo "Starting up everything AmmarServer related.."

scripts/run_myurl&
scripts/run_myloader&
scripts/run_mytube&
scripts/run_myblog&
scripts/run_geoposshare&
scripts/run_mysearch&
scripts/run_pushservice&
 
exit 0


