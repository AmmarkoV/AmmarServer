#!/bin/bash

STARTDIR=`pwd`
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

#RUN ME TO REGENERATE TEMPLATE FILES 

../../StringEscapeFile/StringEscapeFile loginOriginal.html login.html

../../StringEscapeFile/StringEscapeFile 400_.html 400.html
../../StringEscapeFile/StringEscapeFile 401_.html 401.html
../../StringEscapeFile/StringEscapeFile 403_.html 403.html
../../StringEscapeFile/StringEscapeFile 404_.html 404.html
../../StringEscapeFile/StringEscapeFile 408_.html 408.html
../../StringEscapeFile/StringEscapeFile 500_.html 500.html


exit 0
