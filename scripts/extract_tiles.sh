#!/bin/bash

export PATH=./:$PATH

if [ -z "$1" ]; then
	echo usage: $0 '<source dir> <dest dir>'
	exit
fi


if [ -z "$2" ]; then
	echo useage $0 '<source dir> <dest dir>'
	exit
fi

for tile in $( ls $1/*.txt ); do

	filename="${tile##*/}"
	nuclei-extract.py $tile >> $2/$filename 
done 
