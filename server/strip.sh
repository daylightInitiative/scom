#!/usr/bin/bash
#
# utility to remove double comments

echo $1

perl -0777 -pe 's,/\*.*?\*/,,gs' $1
#vim find -type f
