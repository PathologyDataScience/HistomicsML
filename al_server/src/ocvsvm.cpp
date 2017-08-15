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
#include <thread>

#include "ocvsvm.h"



using namespace std;



const float	OCVBinarySVM::SVM_C = 1;
const float	OCVBinarySVM::EPSILON = 1e-3;
const float	OCVBinarySVM::GAMMA = 0.5;



OCVBinarySVM::OCVBinarySVM(void)
{
	m_params.svm_type = CvSVM::C_SVC;
	m_params.kernel_type = CvSVM::RBF;
	m_params.gamma = GAMMA;
	m_params.C = SVM_C;
	m_params.term_crit = cvTermCriteria(CV_TERMCRIT_EPS, 100, EPSILON);
	m_params.coef0 = 0;
	m_params.nu = 0;
	m_params.p = 0.1f;
}




OCVBinarySVM::~OCVBinarySVM(void)
{


}







bool OCVBinarySVM::Train(float *trainSet, int *labelVec,
					  int numObjs, int numDims)
{
	Mat		features(numObjs, numDims, CV_32F, trainSet),
			labels(numObjs, 1, CV_32S, labelVec);

	m_params.gamma = 1.0f / (float)numDims;
	m_trained = m_svm.train(features, labels, Mat(), Mat(), m_params);
	return m_trained;
}







int OCVBinarySVM::Classify(float *obj, int numDims)
{
	int	result = 0;

	if( m_trained ) {
		Mat 	sample(1, numDims, CV_32F, obj);

		result = (int)m_svm.predict(sample);
	}
	return result;
}






bool OCVBinarySVM::ClassifyBatch(float **dataset, int numObjs,
								  int numDims, int *results)
{
	bool	result = false;

	if( m_trained && results != NULL ) {
		for(int i = 0; i < numObjs; i++) {
			Mat	sample(1, numDims, CV_32F, dataset[i]);

			results[i] = (int)m_svm.predict(sample);
		}
		result = true;
	}
	return result;
}





float OCVBinarySVM::Score(float *obj, int numDims)
{
	float	score = 0.0;

	if( m_trained ) {
		Mat 	sample(1, numDims, CV_32F, obj);

		// liopencv seems to return a negated score, compensate appropriately
		score = -m_svm.predict(sample, true);
	}
	return score;
}






bool OCVBinarySVM::ScoreBatch(float **dataset, int numObjs,
							int numDims, float *scores)
{
	bool	result = false;

	if( m_trained && scores != NULL ) {

		vector<std::thread> workers;
		unsigned	numThreads = thread::hardware_concurrency();
		int			offset, objCount, remain, objsPer;

		remain = numObjs % numThreads;
		objsPer = numObjs / numThreads;

		// numThreads - 1 because main thread (current) will process also
		//
		for(int i = 0; i < numThreads - 1; i++) {

			objCount = objsPer + ((i <  remain) ? 1 : 0);
			offset = (i < remain) ? i * (objsPer + 1) :
						(remain * (objsPer + 1)) + ((i - remain) * objsPer);

			workers.push_back(std::thread(&OCVBinarySVM::ScoreWorker, this,
							  std::ref(dataset), offset, objCount, numDims, scores));
		}

		// Main thread's workload
		//
		objCount = objsPer;
		offset = (remain * (objsPer + 1)) + ((numThreads - remain - 1) * objsPer);

		ScoreWorker(dataset, offset, objCount, numDims, scores);

		for( auto &t : workers ) {
			t.join();
		}
		result = true;
	}
	return result;
}






void OCVBinarySVM::ScoreWorker(float **dataset, int offset, int numObjs, int numDims, float *results)
{
	if( m_trained && results != NULL ) {

		for(int i = offset; i < (offset + numObjs); i++) {
			Mat	sample(1, numDims, CV_32F, dataset[i]);
			// liopencv seems to return a negated score, compensate appropriately
			//
			results[i] = -m_svm.predict(sample, true);
		}
	}
}

