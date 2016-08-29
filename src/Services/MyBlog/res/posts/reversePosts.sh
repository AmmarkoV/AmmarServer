#!/bin/bash
 
#DISABLED FOR SAFETY
exit 0

X=0
TOTAL_FRAMES=$1


mkdir reversed 
i=0

echo "Reversing Please wait .. "
for (( i=$X; i<=$TOTAL_FRAMES; i++ ))
do   
  newIndx=$((TOTAL_FRAMES-i)) 
  XNUM=`printf %u $i`
  INUM=`printf %u $newIndx`
   
   
  echo "xxx$XNUM.html -> xxx$INUM.html .. "

  if [ -f "info$XNUM.html" ] 
    then
     cp "info$XNUM.html" "reversed/info$INUM.html"  
  fi 
  if [ -f "post$XNUM.html" ] 
    then
     cp "post$XNUM.html" "reversed/post$INUM.html"  
  fi
  
  echo -n "."
done
  
exit 0
