#!/bin/sh

filename=${0/allindent.sh/emxindent.el}

# find . -name $pat -exec vim -c ":norm 1G=G:wq" {} \;
find . -name \*.cpp -exec emacs -batch {} -l $filename -f my-indent \;
find . -name \*.h -exec emacs -batch {} -l $filename -f my-indent \;
find . -name \*.cxx -exec emacs -batch {} -l $filename -f my-indent \;
