#!/bin/bash

STARTDIR=`pwd`
#Switch to this directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

echo "Ip Tables initially"
sudo iptables -L -n
sudo iptables -F INPUT

input="banlist"
while IFS= read -r var
do
  echo "Banning.. $var"
  sudo iptables -A INPUT -s $var -j DROP
done < "$input"
sudo iptables -L -n

exit 0
