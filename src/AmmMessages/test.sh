#!/bin/bash

  
function generateMSG {
                       ./AmmMessages -msg $1 $2 "/home/nao/mmap"
                       astyle $2.h 
                       mv $2.h output/
                       gcc output/$2.h -o output/$2        
                     }  

cd output 
ln -s ../mmapBridge.h .
ln -s ../mmapBridge.c .
cd ..

generateMSG "samples/msg/person.msg" "person"
generateMSG "samples/msg/skeleton2D.msg" "skeleton2D"
generateMSG "samples/msg/skeleton3D.msg" "skeleton3D"
generateMSG "samples/msg/skeleton2D3D.msg" "skeleton2D3D"
generateMSG "samples/msg/skeletonBBox.msg" "skeletonBBox"
generateMSG "samples/msg/pointEvents.msg" "pointEvents"
 
exit 0
