#!/bin/bash

STARTDIR=`pwd`
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

#RUN ME TO REGENERATE TEMPLATE FILES 

../../StringEscapeFile/StringEscapeFile original/login.html login.html

../../StringEscapeFile/StringEscapeFile original/400.html 400.html
../../StringEscapeFile/StringEscapeFile original/401.html 401.html
../../StringEscapeFile/StringEscapeFile original/403.html 403.html
../../StringEscapeFile/StringEscapeFile original/404.html 404.html
../../StringEscapeFile/StringEscapeFile original/408.html 408.html
../../StringEscapeFile/StringEscapeFile original/500.html 500.html


exit 0
