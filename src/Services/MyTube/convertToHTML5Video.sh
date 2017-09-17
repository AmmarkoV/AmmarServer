#!/bin/bash

echo "Will convert all .$2 files I find in $1 to webm and include it in $3"

 
cd $1

mkdir -p $3

FILES_TO_CONVERT=`ls | grep .$2`
for f in $FILES_TO_CONVERT
do 
 echo "Converting $f" 
 TARGETNAME=`basename $f .$2`
 #-n is for not overwriting 
 ffmpeg -n -i $f -vcodec libvpx -acodec libvorbis $3/$TARGETNAME.webm 
done


exit 0
