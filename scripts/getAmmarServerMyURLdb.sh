#!/bin/bash

DIRTOADD="."

if [ -d src ]
then 
 DIRTOADD="."
fi

if [ -d ../src ]
then 
 DIRTOADD=".."
fi

wget http://ammar.gr/~ammar/myurl.db -O "$DIRTOADD/myurl.db"


exit 0
