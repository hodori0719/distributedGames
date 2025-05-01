#!/bin/sh

find . -name \*.cpp -or -name \*.h -or -name \*.cxx | while read f
do
   perl sanitize.pl $f
done
