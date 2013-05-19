#!/bin/bash

red=$(printf "\033[31m")
green=$(printf "\033[32m") 
normal=$(printf "\033[m")

notFoundItem() 
{
   echo "$red Binary $1 was not found built $normal , please consider running ./get-dependencies.sh to solve build issues"
   echo "If the problem presists create an issue ticket at https://github.com/AmmarkoV/AmmarServer/issues"  
   exit 1 
}

tryToCopy() 
{ 
   #$1 LOCAL PATH , $2 SYSTEM PATH , $3 LOCAL FILENAME 
   if [ -e $1/$3 ]
    then
      echo "$green $3 file is OK including it to system binaries.. $normal"
      sudo cp $1/$3 $2/$3  

      if [ -e $2/$3 ]
       then
         echo "$green Success copying to .. $2/$3 $normal"
       else
         echo "$red Failure copying to .. $2/$3 $normal"
       fi 
    else
      notFoundItem "$3"  
      echo "$red $3 file could not be installed , you probably got a library missing $normal"
      exit 1
    fi   
}
 
 
if [ $( id -u ) -eq 0 ]; then
echo "Will begin installation now"
else
 echo $red
 echo "Installer must be run as root in order to copy files to system.."
 echo "Please re run using sudo ./install.sh , exiting now.."
 echo $normal
 exit 0
fi
 
echo "Installing AmmarServer into system"
sudo echo " "

LIBTARGET="/usr/lib/ammarserver"
BINTARGET="/usr/bin"

if [ -d $LIBTARGET ]
 then
  echo "Found an already install AmmarServer library in $LIBTARGET , overwriting"
 else
  echo "Seems like a fresh installation.."
  sudo mkdir "$LIBTARGET"
fi  

 
#DATA Binaries ( WebServices ) 
tryToCopy "src" "$BINTARGET" "ammarserver" 
tryToCopy "src/MyURL" "$BINTARGET" "myurl"
tryToCopy "src/MyLoader" "$BINTARGET" "myloader"

#DATA Libraries ( Dynamic and Static )  
tryToCopy "src/AmmServerlib" "$LIBTARGET" "libAmmServerlib.so"
tryToCopy "src/AmmServerlib" "$LIBTARGET" "libAmmServerlib.a" 

 
#DATA FILES 
  if [ -e /var/www/index.html ]
  then
    echo "There seems to already be www content in this host skipping copy"
  else
    echo "TODO : Copying templates web page..!"
    sudo cp public_html/* /var/www/
   fi
   
exit 0
