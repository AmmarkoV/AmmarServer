#!/bin/bash

#This project has a small code part ( called Input Parser that is actually a standalone Repository mixed in to the code..
#Since we will go ahead and update_from git , it is good to pull a fresh version out of the github server since bugs may exist
#that will be fixed ( or new ones will be introduced ;P ) .. 
if [ -e "src/AmmServerlib/InputParser/update_from_git.sh" ]; then
  #This means it is not the first we have updated this repository.. so we will just change directory to InputParser and let the ./update_from_git.sh script do its job..   
 cd src/AmmServerlib/InputParser/
 ./update_from_git.sh
 cd ..
 cd ..  
 cd ..  
else
  #This is the first time we are doing this ..  We can understand it since there is no update script ( which should be cloned with the directory ) 
  #We shall clear the embedded InputParser version and clone a fresh one from the repository
  cd src/AmmServerlib/
  rm -rf InputParser
  #Ok now we dont have an InputParser any more.. Lets clone a new one
  git clone http://github.com/AmmarkoV/InputParser.git
  cd ..
  cd ..
fi

#Reminder , to get a single file out of the repo use  "git checkout -- path/to/file.c"


git reset --hard HEAD
git pull
./make
exit 0
