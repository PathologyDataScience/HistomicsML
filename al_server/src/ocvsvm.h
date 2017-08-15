//
//	Copyright (c) 2014-2015, Emory University
//	All rights reserved.
//
//	Redistribution and use in source and binary forms, with or without modification, are
//	permitted provided that the following conditions are met:
//
//	1. Redistributions of source code must retain the above copyright notice, this list of
//	conditions and the following disclaimer.
//
//	2. Redistributions in binary form must reproduce the above copyright notice, this list
// 	of conditions and the following disclaimer in the documentation and/or other materials
//	provided with the distribution.
//
//	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//	SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
//	WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//	DAMAGE.
//
//
#if !defined(_OCVSVM_H_)
#define _OCVSVM_H_

#include <opencv2/core/core.hpp>
#include <opencv2/ml/ml.hpp>

#include "classifier.h"



using namespace cv;




class OCVBinarySVM : public Classifier
{
public:

					OCVBinarySVM(void);
	virtual			~OCVBinarySVM(void);


	virtual int 	Classify(float *obj, int numDims);
	virtual bool	ClassifyBatch(float **dataset, int numObjs,
								  int numDims, int *results);

	virtual bool	Train(float *trainSet, int *labelVec,
						  int numObjs, int numDims);

	virtual float	Score(float *obj, int numDims);
	virtual bool	ScoreBatch(float **dataset, int numObjs,
								int numDims, float *scores);


protected:

	static const float		SVM_C;
	static const float		EPSILON;
	static const float		GAMMA;


	CvSVMParams		m_params;
	CvSVM			m_svm;

	void	ScoreWorker(float **data, int offset, int numObjs, int numDims, float *results);
};




#endif
