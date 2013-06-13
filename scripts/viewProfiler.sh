#!/bin/bash


PROJECT_TO_VIEW="ammarserver"

if [ $# -ne 1 ]
then 
  echo "Using default concurrent connection number"
else
 PROJECT_TO_VIEW=$1 
fi

gprof src/$PROJECT_TO_VIEW > "$PROJECT_TO_VIEW"Profiling.txt

gedit "$PROJECT_TO_VIEW"Profiling.txt
 
exit 0
