#!/bin/bash

STARTDIR=`pwd`
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

#RUN ME TO REGENERATE TEMPLATE FILES 

../../StringEscapeFile/StringEscapeFile original/login.html include/login.html 
../../StringEscapeFile/StringEscapeFile original/400.html include/400.html
../../StringEscapeFile/StringEscapeFile original/401.html include/401.html
../../StringEscapeFile/StringEscapeFile original/403.html include/403.html
../../StringEscapeFile/StringEscapeFile original/404.html include/404.html
../../StringEscapeFile/StringEscapeFile original/408.html include/408.html
../../StringEscapeFile/StringEscapeFile original/500.html include/500.html


../../FileToString/FileToString original/directoryListStart.html include/directoryListStart.html
../../StringEscapeFile/StringEscapeFile original/directoryListEnd.html include/directoryListEnd.html


../../FileToString/FileToString original/dir.gif include/dir.html
../../FileToString/FileToString original/doc.gif include/doc.html
../../FileToString/FileToString original/exe.gif include/exe.html
../../FileToString/FileToString original/img.gif include/img.html
../../FileToString/FileToString original/vid.gif include/vid.html
../../FileToString/FileToString original/mus.gif include/mus.html
../../FileToString/FileToString original/back.gif include/back.html
../../FileToString/FileToString original/up.gif include/up.html

exit 0
