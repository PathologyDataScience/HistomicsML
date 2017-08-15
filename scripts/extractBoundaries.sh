#!/bin/bash

export PATH=./:$PATH

if [ -z "$1" ]; then
	echo usage: $0 '<source dir> <dest file>'
	exit
fi


if [ -z "$2" ]; then
	echo useage $0 '<source dir> <dest file>'
	exit
fi


for slide in $( find $1 -maxdepth 1 -type d ); do
	echo $slide
    for tile in $( ls $slide/*.seg*.txt ); do
		echo $tile
		nuclei-extract.py $tile >> $2
	done
done 

