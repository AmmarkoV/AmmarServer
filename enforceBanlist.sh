#!/bin/bash

STARTDIR=`pwd`
#Switch to this directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"


input="banlist"


#Get Bad IPs from error logs
cat log/*_error.log | grep -vE '127\.0\.0\.1|STRINGHERE|0\.0\.0\.0' | cut -d ' ' -f 1 | sort | uniq -u > banNew
#Add already known bad IPs 
cat $input >> banNew
#Keep everything only once
cat banNew | uniq -u > $input
 
echo "Ip Tables initially"
sudo iptables -L -n
sudo iptables -F INPUT

while IFS= read -r var
do
  echo "Banning.. $var"
  sudo iptables -A INPUT -s $var -j DROP
done < "$input"
sudo iptables -L -n

exit 0
