#!/bin/bash

STARTDIR=`pwd`
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"


../../StringEscapeFile/StringEscapeFile loginOriginal.html login.html

exit 0
