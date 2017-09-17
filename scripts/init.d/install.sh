#!/bin/bash
 
STARTDIR=`pwd`
#Switch to this directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

sudo cp ammarserver /etc/init.d/ammarserver
sudo touch /var/log/ammarserver.log
sudo chown ammar /var/log/ammarserver.log
sudo update-rc.d ammarserver defaults

exit 0
