#!/bin/bash

echo "Starting up everything AmmarServer related.."

./run_myurl&
./run_myloader&
./run_mytube&
./run_myblog&
./run_geoposshare&
./run_mysearch&
 
exit 0


