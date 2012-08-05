#!/bin/bash

BINARY="ammarserver"

  if [ -e /usr/bin/$BINARY ]
  then
    sudo rm  /usr/bin/$BINARY 
  else  
    echo "No $BINARY detected "
  fi
 
exit 0