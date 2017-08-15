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
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cstring>
#include <cfloat>
#include <ctime>
#include <cmath>
#include <algorithm>

#include "sampler.h"

#include "logger.h"


extern EvtLogger *gLogger;

using namespace std;


Sampler::Sampler(MData *dataset) :
m_dataIndex(NULL),
m_remaining(0),
m_dataset(dataset)
{
	srand(time(NULL));
}





Sampler::~Sampler(void)
{
	if( m_dataIndex )
		free(m_dataIndex);
}



//
//	Let the sampler know which samples were selected
// 	by the user during priming.
//
bool Sampler::Init(int count, int *list)
{
	bool	result = true;
	int		pick;

	for(int i = 0; i < count; i++) {
		for(int j = 0; j < m_remaining; j++) {
			if( m_dataIndex[j] == list[i] ) {
				pick = j;
				break;
			}
		}
		m_remaining--;
		m_dataIndex[pick] = m_dataIndex[m_remaining];
	}
	return result;
}





//-----------------------------------------------------------------------------


UncertainSample::UncertainSample(Classifier *classify, MData *dataset) : Sampler(dataset),
m_Classify(classify)
{
	int	numObjs = dataset->GetNumObjs();
	m_dataIndex = (int*)malloc(numObjs * sizeof(int));

	m_checkSet = (float**)calloc(numObjs, sizeof(float*));

 	if( m_checkSet ) {
		
		// The checkset is just the unlabled portion of the dataset. It starts out
		// as the entire dataset and as objects are labled, they are removed from the
		// checkset. We only need an array of pointers to do this. The actual object 
		// data will be stored in the dataset object, we will just swap out the pointers 
		// for selected objects in our local array of pointers.
		//
		float **data = dataset->GetData();

		for(int i = 0; i < numObjs; i++) {
			m_checkSet[i] = data[i];
		}
	}

	if( m_dataIndex ) {
		m_remaining = numObjs;
		for(int i = 0; i < m_remaining; i++)
			m_dataIndex[i] = i;
	}

	m_slideCnt = (float*)calloc(dataset->GetNumSlides(), sizeof(float));
}




UncertainSample::~UncertainSample(void)
{
	if( m_slideCnt )
		free(m_slideCnt);
	if( m_checkSet )
		free(m_checkSet);
}




bool UncertainSample::Init(int count, int *list)
{
	bool	result = false;

	// Initialize the slide counts with the 'primed' objects then
	// call the base constructor to finish the initialization.
	if( m_slideCnt ) {
		int	*slideIdx = m_dataset->GetSlideIndices();

		for(int i = 0; i < count; i++) {
			m_slideCnt[slideIdx[list[i]]]++;
		}
		result = true;
	}

	if( result )
		result = Sampler::Init(count, list);

	if( result ) {
		int	numObjs = m_dataset->GetNumObjs();

		// Remove the initial objects from the checkset
		for(int i = 0; i < count; i++) {
			numObjs--;
			m_checkSet[i] = m_checkSet[numObjs];
		}
	}

	return result;
}






// Select the most uncertain object in the remaining set. The score returned by
// ScoreBatch is the conditional entropy for the object. The object with the
// maximum conditional entropy is the most uncertain.
//
int UncertainSample::Select(float *score)
{
	int		pick = -1;
	float	*scores = (float*) malloc(m_remaining * sizeof(float));

	if( scores ) {
		if( m_Classify->ScoreBatch(m_checkSet, m_remaining, m_dataset->GetDims(), scores) ) {
			int minIdx = -1;
			float  min = FLT_MAX;

			for(int i = 0; i < m_remaining; i++) {

				if( abs(scores[i]) < min ) {
					min = abs(scores[i]);
					minIdx = i;
				}
			}

			if( score != NULL )
				*score = scores[minIdx];

			pick = m_dataIndex[minIdx];
			m_remaining--;
			m_dataIndex[minIdx] = m_dataIndex[m_remaining];
		}
		free(scores);
	}
	return pick;
}






// Select the 'count' most uncertain samples in the remaining set. The id's
// and score buffers are allcated here but must be freed by the caller.
//
bool UncertainSample::SelectBatch(int count, int *&ids, float *&selScores)
{
	bool	result = true;
	float	*scores = (float*)malloc(m_remaining * sizeof(float));
	int		*picks = (int*)malloc(count * sizeof(int)), sign;

	ids = (int*)malloc(count * sizeof(int));
	selScores = (float*)malloc(count * sizeof(float));

	if( ids == NULL || selScores == NULL || scores == NULL || picks == NULL ) {
		result = false;
	}


	if( result ) {
		if( !m_Classify->ScoreBatch(m_checkSet, m_remaining, m_dataset->GetDims(), scores) ) {
			result = false;
		}
	}

	if( result ) {
		int minIdx, *slideIdx = m_dataset->GetSlideIndices(),
					sampleCnt = m_dataset->GetNumObjs() - m_remaining;
		float  min = FLT_MAX, objScore, weight;

		for(int i = 0; i < count; i++) {
			selScores[i] = FLT_MAX;
		}

		for(int i = 0; i < m_remaining; i++) {
			objScore = abs(scores[i]);
			minIdx = count - 1;

#if defined(USE_WEIGHT)
			// Add in slide weight
			if( sampleCnt > 0 ) {
				weight = (m_slideCnt[slideIdx[m_dataIndex[i]]] / (float)sampleCnt);
				objScore += weight;
			}
#endif
			if( objScore < abs(selScores[minIdx]) ) {
				// Score is less than at least the last element, put it
				// there to start

				while( minIdx > 0 && objScore < abs(selScores[minIdx - 1]) ) {
					selScores[minIdx] = selScores[minIdx - 1];
					picks[minIdx] = picks[minIdx - 1];
					minIdx--;
				}
				selScores[minIdx] = scores[i];
#if defined(USE_WEIGHT)
				sign = (scores[i] < 0) ? -1 : 1;
				selScores[minIdx] += (weight * sign);
#endif
				picks[minIdx] = i;
			}
		}

		// Check if the uncertainty score varies... If not we may need to "spread out"
		// the object selection among the available slides.
		//
		if( selScores[0] == selScores[count - 1] ) {
			vector<int>	minObjs;
#if defined(USE_WEIGHT)
			set<int>	repSlides;
#endif
			// All scores have the same uncertainty. Get all objects with a
			// score equal to the min and TODO select objects from as many slides
			// as possible. Note - For now we are just selecting randomly from
			// the objects with the min value.
			//
			for(int i = 0; i < m_remaining; i++) {
				objScore = abs(scores[i]);
#if defined(USE_WEIGHT)
				weight = (m_slideCnt[slideIdx[m_dataIndex[i]]] / (float)sampleCnt);
				objScore += weight;
#endif
				if( objScore == selScores[0] ) {
					minObjs.push_back(i);
#if defined(USE_WEIGHT)
					repSlides.insert(slideIdx[m_dataIndex[i]]);
#endif
				}
			}

			gLogger->LogMsg(EvtLogger::Evt_INFO, "Uncertain obj count: %d", minObjs.size());
			minIdx = 0;
			for(int i = 0; i < count; i++ ) {
				int obj = rand() % minObjs.size();
				picks[minIdx++] = minObjs[obj];
				minObjs.erase(minObjs.begin() + obj);
			}
		}

		// Remove selected objects from checkset
		for(int i = 0; i < count; i++) {
			m_remaining--;
			ids[i] = m_dataIndex[picks[i]];
			m_slideCnt[slideIdx[ids[i]]]++;
			m_dataIndex[picks[i]] = m_dataIndex[m_remaining];
			m_checkSet[picks[i]] = m_checkSet[m_remaining];
		}
	}

	if( picks )
		free(picks);
	if( scores )
		free(scores);

	return result;
}






struct ScoreIdx{
	int	idx;
	float score;
};

static bool SortFunc(ScoreIdx a, ScoreIdx b)
{
	return (a.score < b.score);
}




//
//	Select samples for visualization. nStrata specifies the number of uncertainty
// 	levels. An nStrata of 1 will select the most uncertain sample, 2 will select
//  the most uncertain and the most certain, 3 will add the 50% percentile and
//  so on. nGroups specifies the number of samples per strata. Samples will be
//	returned for both clases, so for an nStrata of 4 and nGroups of 2 there will
//	be 4(strata) * 2(groups) * 2(classes) samples for a total of 16 returned.
//	They are ordered as follows: Group 1 Neg class most certain to most uncertain,
//	Group 1 Pos class most uncertain to most certain, ... Group N Neg class most
//	certain to most uncertain, Group N Pos class most uncertain to most certain.
//
//
bool UncertainSample::GetVisSamples(int nStrata, int nGroups, int *&idx, float *&idxScores)
{
	bool	result = true;
	float	*scores = (float*) malloc(m_remaining * sizeof(float));


	idx = (int*)calloc(nStrata * nGroups * 2, sizeof(int));
	idxScores = (float*)calloc(nStrata * nGroups * 2, sizeof(float));

	// Score the unlabeled data and split into 2 classes
	if( scores && idxScores && idx ) {

		vector<ScoreIdx> neg, pos;
		ScoreIdx	temp;

		if( m_Classify->ScoreBatch(m_checkSet, m_remaining, m_dataset->GetDims(), scores) ) {

			for(int i = 0; i < m_remaining; i++) {
				if( scores[i] < 0 ) {
					temp.score = scores[i];
					temp.idx = m_dataIndex[i];
					neg.push_back(temp);
				} else {
					temp.score = scores[i];
					temp.idx = m_dataIndex[i];
					pos.push_back(temp);
				}
			}

			sort(neg.begin(), neg.end(), SortFunc);
			sort(pos.begin(), pos.end(), SortFunc);

			float posPercent[nStrata], negPercent[nStrata];

			posPercent[0] = negPercent[0] = 0;
			posPercent[nStrata - 1] = pos.size() - nGroups - 1;
			negPercent[nStrata - 1] = neg.size() - nGroups - 1;

			float stride = (1.0f / (float)(nStrata - 1));
			for(int i = 1; i < nStrata - 1; i++) {
				posPercent[i] = i * stride * pos.size();
				negPercent[i] = i * stride * neg.size();
			}

			for(int grp = 0; grp < nGroups; grp++) {
				for(int s = 0; s < nStrata; s++) {
					idx[(grp * 2 * nStrata) + s] = neg[negPercent[s] + grp].idx;
					idx[(grp * 2 * nStrata) + s + nStrata] = pos[posPercent[s] + grp].idx;

					if( idxScores ) {
						idxScores[(grp * 2 * nStrata) + s] = neg[negPercent[s] + grp].score;
						idxScores[(grp * 2 * nStrata) + s + nStrata] = pos[posPercent[s] + grp].score;
					}
				}
			}
		}
	}

	if( scores )
		free(scores);

	return result;
}



//------------------------------------------------------------------------------



UncertRandomSample::UncertRandomSample(Classifier *classify, MData *dataset) : UncertainSample(classify, dataset)
{
	int	numObjs = dataset->GetNumObjs();

	m_sampleSize = (int)((float)numObjs * 0.10f);
	m_sampledSet = (float**)calloc(m_sampleSize, sizeof(float*));
	m_sampleIndex = (int*)malloc(m_sampleSize * sizeof(int));

}




UncertRandomSample::~UncertRandomSample(void)
{
	if( m_sampledSet )
		free(m_sampledSet);
	if( m_sampleIndex )
		free(m_sampleIndex);
}





bool UncertRandomSample::SelectBatch(int count, int *&ids, float *&selScores)
{
	bool	result = true;
	float	*scores = (float*)malloc(m_sampleSize * sizeof(float));
	int		*picks = (int*)malloc(count * sizeof(int)), sign;

	ids = (int*)malloc(count * sizeof(int));
	selScores = (float*)malloc(count * sizeof(float));

	if( ids == NULL || selScores == NULL || scores == NULL || picks == NULL ) {
		result = false;
	}

	if( result ) {
		result = UpdateCheckSet();
	}

	if( result ) {
		if( !m_Classify->ScoreBatch(m_sampledSet, m_sampleSize, m_dataset->GetDims(), scores) ) {
			result = false;
		}
	}

	if( result ) {
		int minIdx, *slideIdx = m_dataset->GetSlideIndices(),
					sampleCnt = m_dataset->GetNumObjs() - m_remaining;
		float  min = FLT_MAX, objScore;

		for(int i = 0; i < count; i++) {
			selScores[i] = FLT_MAX;
		}

		for(int i = 0; i < m_sampleSize; i++) {
			objScore = abs(scores[i]);
			minIdx = count - 1;

			if( objScore < abs(selScores[minIdx]) ) {
				// Score is less than at least the last element, put it
				// there to start

				while( minIdx > 0 && objScore < abs(selScores[minIdx - 1]) ) {
					selScores[minIdx] = selScores[minIdx - 1];
					picks[minIdx] = picks[minIdx - 1];
					minIdx--;
				}
				selScores[minIdx] = scores[i];
				picks[minIdx] = i;
			}
		}

		// Check if the uncertainty score varies... If not we may need to "spread out"
		// the object selection among the available slides.
		//
		if( selScores[0] == selScores[count - 1] ) {
			vector<int>	minObjs;
			set<int>	repSlides;

			// All scores have the same uncertainty. Get all objects with a
			// score equal to the min and TODO select objects from as many slides
			// as possible. Note - For now we are just selecting randomly from
			// the objects with the min value.
			//
			for(int i = 0; i < m_sampleSize; i++) {
				objScore = abs(scores[i]);

				if( objScore == selScores[0] ) {
					minObjs.push_back(i);
				}
			}

			gLogger->LogMsg(EvtLogger::Evt_INFO, "Uncertain obj count: %d", minObjs.size());
			minIdx = 0;
			for(int i = 0; i < count; i++ ) {
				int obj = rand() % minObjs.size();
				picks[minIdx++] = minObjs[obj];
				minObjs.erase(minObjs.begin() + obj);
			}
		}

		// Remove selected objects from checkset
		for(int i = 0; i < count; i++) {
			m_remaining--;
			ids[i] = m_sampleIndex[picks[i]];
			m_dataIndex[picks[i]] = m_dataIndex[m_remaining];	// m_sampleIndex contains indices into m_dataIndex
			m_checkSet[picks[i]] = m_checkSet[m_remaining];
		}
	}

	if( picks )
		free(picks);
	if( scores )
		free(scores);
	return result;
}





bool UncertRandomSample::UpdateCheckSet(void)
{
	bool	result = false;
	int		left, pick, *index = (int*)malloc(m_remaining * sizeof(int));

	if( m_sampleIndex != NULL && 
		index != NULL &&
		m_sampleSize <= m_remaining ) {
		
		left = m_remaining;
		for(int i = 0; i < m_remaining; i++) 
			index[i] = m_dataIndex[i];

		for(int i = 0; i < m_sampleSize; i++) {
			
			pick = rand() % left;
			m_sampleIndex[i] = index[pick];
			m_sampledSet[i] = m_checkSet[index[pick]];

			left--;
			index[pick] = index[left];
		}
		result = true;
	}
	
	if( index )
		free(index);

	return result;
}





//------------------------------------------------------------------------------

DistSample::DistSample(Classifier *classify, MData *dataset) : 
	UncertainSample(classify, dataset)
{

}


DistSample::~DistSample(void)
{

}




bool DistSample::SelectBatch(int count, int *&ids, float *&selScores)
{
	bool	result = true;
	float	*scores = (float*)malloc(m_remaining * sizeof(float));
	int		*picks = (int*)malloc(count * sizeof(int)), sign;

	ids = (int*)malloc(count * sizeof(int));
	selScores = (float*)malloc(count * sizeof(float));

	if( ids == NULL || selScores == NULL || scores == NULL || picks == NULL ) {
		result = false;
	}


	if( result ) {
		if( !m_Classify->ScoreBatch(m_checkSet, m_remaining, m_dataset->GetDims(), scores) ) {
			result = false;
		}
	}


	vector<int> scoreBins[8];
	float	binDefs[8] = { -0.5f, -0.25f, 0.0f, 0.1f, 0.25f, 0.3f, 0.5f, 1.0f };

	if( result ) {
		int 	idx, bin;
		for(int i = 0; i < m_remaining; i++) {
	
			//idx = (int)floor((scores[i] + 1.0f) * 4.0f);

			for(idx = 0; idx < 8; idx++) {
				if( scores[i] <= binDefs[idx] ) {
					break;
				}
			}
			scoreBins[idx].push_back(i);
		}
		
		// Randomly select one from each bin
		for(int i = 0; i < 8; i++) {
			gLogger->LogMsg(EvtLogger::Evt_INFO, "bin %d, has %d samples", i, scoreBins[i].size());

			bin = i;
			while( scoreBins[bin].size() == 0 ) {
				bin = (bin + 1) % 8;
			}
			idx = rand() % scoreBins[bin].size();
			ids[i] = m_dataIndex[scoreBins[bin][idx]];
			//selScores[i] = ((float)bin / 4.0f) - 1.0f;
			selScores[i] = binDefs[bin];
			m_remaining--;
			m_dataIndex[scoreBins[i][idx]] = m_dataIndex[m_remaining];
			m_checkSet[scoreBins[i][idx]] = m_checkSet[m_remaining];

		}
	}
	if( picks )
		free(picks);
	if( scores )
		free(scores);

	return result;
}




//------------------------------------------------------------------------------


RandomSample::RandomSample(MData *dataset) : Sampler(dataset)
{

	int numObjs = dataset->GetNumObjs();
	m_dataIndex = (int*)malloc(numObjs * sizeof(int));
	if( m_dataIndex ) {
		m_remaining = numObjs;
		for(int i = 0; i < numObjs; i++)
			m_dataIndex[i] = i;
	}
}






RandomSample::~RandomSample(void)
{

}






// Selects a random object without replacement.
// Returns -1 if no objects are left
//
int RandomSample::Select(float *score)
{
	int	obj = -1, pick;

	if( m_dataIndex && m_remaining > 0 ) {
		pick = rand() % m_remaining;
		obj = m_dataIndex[pick];

		// Put last obj of list in spot just selected
		m_remaining--;
		m_dataIndex[pick] = m_dataIndex[m_remaining];
	}
	return obj;
}







bool RandomSample::SelectBatch(int count, int *&ids, float *&scores)
{
	bool	result = false;

	ids = (int*)malloc(count * sizeof(int));
	scores = (float*)malloc(count * sizeof(float));

	if( ids != NULL && scores != NULL ) {
		for(int i = 0; i < count; i++) {
			ids[i] = Select(&scores[i]);
		}
		result = true;
	}	
	return result;
}


