#!/bin/bash
 
STARTDIR=`pwd`
#Switch to this directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

sudo cp init.d/ammarserver /etc/init.d/ammarserver
sudo touch /var/log/ammarserver.log
sudo chown ammar /var/log/ammarserver.log
sudo update-rc.d ammarserver defaults

if [ -e /lib/systemd/system/ ]
then
 #systemd?
 echo "SystemD bloat detected, registering with it.."
 sudo systemctl daemon-reload
 sudo cp systemd/ammarserver.service /lib/systemd/system/
fi


exit 0
