#!/bin/bash
#
#	Copyright (c) 2014-2017, Emory University
#	All rights reserved.
#
#	Redistribution and use in source and binary forms, with or without modification, are
#	permitted provided that the following conditions are met:
#
#	1. Redistributions of source code must retain the above copyright notice, this list of
#	conditions and the following disclaimer.
#
#	2. Redistributions in binary form must reproduce the above copyright notice, this list
# 	of conditions and the following disclaimer in the documentation and/or other materials
#	provided with the distribution.
#
#	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
#	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
#	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
#	SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
#	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
#	WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
#	DAMAGE.
#
#

export PATH=./:$PATH

if [ -z "$1" ]; then
	echo usage: $0 '<source dir> <dest file>'
	exit
fi


if [ -z "$2" ]; then
	echo useage $0 '<source dir> <dest file>'
	exit
fi


for slide in $( ls $1/*.seg*.txt ); do
	echo $slide;
	extract-GBM-nuclei.py $slide >> $2
done
