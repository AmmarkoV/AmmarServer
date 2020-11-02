#!/bin/bash

STARTDIR=`pwd`
#Switch to this directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

sudo apt-get install libssl-dev

cd ..
mkdir key
cd key
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365

exit 0

