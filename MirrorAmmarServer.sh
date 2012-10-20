#!/bin/bash
#This is a script to help you mirror your dynamic site in order to get convert to static pages that can be served with AmmarServer  
 wget --mirror -p --html-extension --convert-links -e robots=off --restrict-file-names=windows -P . $1
exit 0