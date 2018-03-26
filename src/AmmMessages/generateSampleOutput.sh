#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

MMAPDIR="/home/ammar/mmap"  

function generateMSG {
                       ./AmmMessages -msg $1 $2 $MMAPDIR
                       astyle $2.h 
                       mv $2.h output/
                       gcc output/$2.h -o output/$2   
                       rm output/$2   
                     }  

cd output 
ln -s ../mmapBridge.h .
ln -s ../mmapBridge.c .
cd ..

generateMSG "samples/msg/person.msg" "person" 
generateMSG "samples/msg/skeleton2D3D.msg" "skeleton2D3D" 
generateMSG "samples/msg/pointEvents.msg" "pointEvents"
generateMSG "samples/msg/behavior.msg" "behavior"
generateMSG "samples/msg/say.msg" "say"
generateMSG "samples/msg/move.msg" "move"
generateMSG "samples/msg/command.msg" "command"
generateMSG "samples/msg/nav.msg" "nav"
generateMSG "samples/msg/step.msg" "step"
generateMSG "samples/msg/odometry.msg" "odometry"
generateMSG "samples/msg/state.msg" "state"
 
 
./AmmMessages -gather person skeleton2D3D pointEvents behavior say move command step odometry state
astyle allAmmMessages.h 
mv allAmmMessages.h output/

rm *.orig

exit 0
