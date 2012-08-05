#!/bin/bash

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
 
   
  if [ -e /srv/httptemplates/404.html ]
  then
    echo "There seems to already be template content in this host skipping copy"
  else
    echo "TODO : Copying templates web page..!"
    sudo cp /robot/permfs/www/guarddog.jpg  /srv/http/guarddog.jpg
    sudo cp /robot/permfs/www/StaticPlaceholderPage.html  /srv/http/index.html
   fi
   
exit 0