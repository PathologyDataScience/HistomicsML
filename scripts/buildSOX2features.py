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
import h5py
import os
import glob


if len(sys.argv) != 3:
	print "Usage:", sys.argv[0], "<slides dir> <out file>"
	exit(1)

# SOX2 features have 3 dimensions
dims = 3


slideCnt = 0
objectCnt = 0

outFile = h5py.File(sys.argv[2], "w")
x_centroids = outFile.create_dataset("x_centroid", (0,), maxshape=(1000000000,))
y_centroids = outFile.create_dataset("y_centroid", (0,), maxshape=(1000000000,))
slideIdx = outFile.create_dataset("slideIdx", (0,), maxshape=(1000000000,), dtype='i4')
dt = h5py.special_dtype(vlen=bytes)
slideNames = outFile.create_dataset("slides", (0,), maxshape=(10000,), dtype=dt) 
dataIdx = outFile.create_dataset("dataIdx", (0,), maxshape=(10000,), dtype='i4')
features = outFile.create_dataset("features", (0,dims), maxshape=(1000000000,dims))


slides = os.listdir(sys.argv[1])
for slide in slides:
	if os.path.isfile(slide):
		print "Skipping", slide
		continue
	
	print "Adding: ", slide
	tiles = glob.glob(sys.argv[1]+ slide + "/*.seg.*.txt")
	slideNames.resize((slideCnt+1,))
	slideNames[slideCnt] = slide
	
	# 
	# Index where features for current slide start
	#
	dataIdx.resize((slideCnt+1,))
	dataIdx[slideCnt] = objectCnt;

	for tile in tiles:

		
		data = open(tile, 'r').readlines()
		data = data[1:]
		objs = len(data)
		newCnt = objectCnt + objs
		print "Resizing to", newCnt
		
		# Resize datasets 
		features.resize((newCnt, dims))
		x_centroids.resize((newCnt,))
		y_centroids.resize((newCnt,))
		slideIdx.resize((newCnt,))
		
		for obj in range(objs):
			slideIdx[objectCnt + obj] = slideCnt
			
			values = string.split(data[obj], '\t')
			print "centroid", values[1], values[2], "at", objectCnt + obj
			
			#
			#	Add magnification check when segmentation files are fixed. (Currently
			#	the analysis & scan magnification are missing from the output)
			#
			x_centroids[objectCnt + obj] = float(values[1]) * 2
			y_centroids[objectCnt + obj] = float(values[2]) * 2
			
			objFeatures = np.array([float(i) for i in values[3:dims + 3]])
			features[objectCnt + obj] = objFeatures

		objectCnt = objectCnt + objs

				
	slideCnt = slideCnt + 1
		
	
outFile.close()
