#!/usr/bin/env python
#
#	Copyright (c) 2014-2015, Emory University
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
import sys
import string
import numpy as np

dims = 48


if len(sys.argv) != 2:
	print "Usage:", sys.argv[0], " <segmentation file> "
	exit(1)


data = open(sys.argv[1], 'r').readlines()
data = data[1:]   		# Remove header


for nuclei in data:

	items = string.split(nuclei, '\t')

	#
	##	Check for valid features, skip if not
	#	
	objFeatures = np.array([float(i) for i in items[5:dims + 5]])
	if np.isnan(np.sum(objFeatures)) == True:
		continue
			
	#
	# Remove extra from slide name
	nameSplit = string.split(items[0], '.')
	
	if float(items[3]) == 40.0:
		#
		#	Need to adjust for 40x slides by multiplying by 2
		#
		x = float(items[1]) * 2.0
		y = float(items[2]) * 2.0

		print nameSplit[0], "\t", x, "\t", y, "\t", 
		vertices = string.split(items[len(items) - 1], ';')

		#
		#	Strip semicolons
		#
		for point in vertices:

			coords = string.split(point, ',')
			if len(coords) == 2:
				x = float(coords[0]) * 2.0
				y = float(coords[1]) * 2.0
				print  str(x) + "," + str(y),
		print

	else:

		print nameSplit[0], "\t", items[1], "\t", items[2], "\t",
		vertices = string.split(items[len(items) - 1], ';')

		#
		#	Strip semicolons
		#
		for point in vertices:
			coords = string.split(point, ',')
			if len(coords) == 2:
				print point[0:point.index('.')] + point[point.index('.')+2:-2],
	
		print



