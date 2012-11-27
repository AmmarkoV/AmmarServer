#!/bin/bash

echo "System Installation is deactivated.. I havent decided on where to put content yet.."
exit 0

echo "Installing AmmarServer into system"
sudo echo " "

BINPATH="src"
BINARY="ammarserver"
  if [ -e $BINPATH/$BINARY ]
  then
    echo "$BINARY App is OK :) , including it to system binaries .."
    sudo cp $BINPATH/$BINARY /usr/bin/$BINARY 
  else
    echo "$BINARY App could not be installed , you probably got a library missing"
    exit 1
  fi
 
   
  if [ -e /var/www/index.html ]
  then
    echo "There seems to already be www content in this host skipping copy"
  else
    echo "TODO : Copying templates web page..!"
    sudo cp public_html/* /var/www/
   fi
   
exit 0