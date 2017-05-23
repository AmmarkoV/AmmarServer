#!/bin/bash

#SERVER TO SET UP :)
#See Apache_000-default.conf for what it is going to look like after this script..
SERVERURL="ammar.gr"  #Server "public url" for reverse proxy
APCONF="/etc/apache2/sites-availiable/default" #sites-availiable file that will contain the new virtual hosts added


function addApacheRule 
{ 
if cat $1 | grep -q "$3"
then
   echo "$3 set-up ok!" 
else
   echo "Setting-up $3!" 
   sudo sh -c "echo ' ' >> $1"
   sudo sh -c "echo '<VirtualHost *:80>' >> $1"
   sudo sh -c "echo 'ServerName \"$3.$2\"' >> $1"
   sudo sh -c "echo 'ProxyPreserveHost On' >> $APCONF"
   sudo sh -c "echo 'ProxyPass / \"http://$2:$4/\"' >> $1"
   sudo sh -c "echo 'ProxyPassReverse / \"http://$SERVERURL:8085/\"' >> $1"
   sudo sh -c "echo '</VirtualHost>' >> $1"
   sudo sh -c "echo ' ' >> $1"
fi
}  

echo "This is an automated script to modify apache2 settings and add ammarserver to my main machine " 
echo "It will probably wont do what you want in any machine it is best to take a look at the script  " 
echo "Are you sure you want to run it ?  " 

 
  echo
  echo -n " (Y/N)?"
  read answer
  if test "$answer" != "N" -a "$answer" != "n";
  then 
    echo "Going ahead..!"
  else
   exit 0
  fi


if [ -d /etc/apache2/sites-availiable ] 
   then
     echo "Apache2 found"
   else
     echo "Could not find an apache2 installation"
     exit 0
   fi


if [ -f /etc/apache2/sites-availiable/default ]
then 
 echo "Found default site , will append it"
else 
 echo "Could not find default site to append it"
 exit 0  
fi


echo "The following require a2enmod proxy " 
echo "and                   a2enmod proxy_http"
sudo a2enmod proxy
sudo a2enmod proxy_http

 
addApacheRule "$APCONF" "$SERVERURL" "mytube" "8080"  
addApacheRule "$APCONF" "$SERVERURL" "gps" "8081"  
addApacheRule "$APCONF" "$SERVERURL" "myurl" "8082"  
addApacheRule "$APCONF" "$SERVERURL" "chan" "8083"  
addApacheRule "$APCONF" "$SERVERURL" "search" "8084"  
addApacheRule "$APCONF" "$SERVERURL" "myloader" "8085"  
addApacheRule "$APCONF" "$SERVERURL" "blog" "8086"  
   

echo "and of course a       service apache2 restart"
sudo service apache2 restart


exit 0
