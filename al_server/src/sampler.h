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
#if !defined(_SAMPLER_H_)
#define _SAMPLER_H_

#include "classifier.h"
#include "data.h"





class Sampler
{
public:
				Sampler(MData *dataset);
	virtual		~Sampler(void);

	enum SampType {SAMP_RANDOM, SAMP_UNCERTAIN, SAMP_INFO_DENSITY, SAMP_HIERARCHICAL, SAMP_CLUSTER};

	virtual int 	Select(float *score = NULL) = 0;
	virtual bool	SelectBatch(int count, int *&ids, float *&scores) = 0;
	virtual bool 	Init(int count, int *list);
	virtual SampType GetSamplerType(void) = 0;
	virtual bool 	GetVisSamples(int nStrata, int nGroups, int *&idx, float *&idxScores) = 0;

protected:

	MData	*m_dataset;
	int		*m_dataIndex;
	int		m_remaining;

};





//-----------------------------------------------------------------------------




class UncertainSample : public Sampler
{
public:
				UncertainSample(Classifier *classify, MData *dataset);
	virtual		~UncertainSample(void);

	virtual int		Select(float *score = NULL);
	virtual bool	SelectBatch(int count, int *&ids, float *&scores);
	virtual bool 	Init(int count, int *list);
	virtual SampType 	GetSamplerType(void) { return SAMP_UNCERTAIN; }
	virtual bool 	GetVisSamples(int nStrata, int nGroups, int *&idx, float *&idxScores);

protected:


	float	*m_slideCnt;
	float	**m_checkSet;

	Classifier 	*m_Classify;

};





class RandomSample : public Sampler
{
public:
				RandomSample(MData *dataset);
	virtual 	~RandomSample(void);

	virtual int 		Select(float *score = NULL);
	virtual bool		SelectBatch(int count, int *&ids, float *&scores);
	virtual SampType	GetSamplerType(void) { return SAMP_RANDOM; }

	virtual bool 	GetVisSamples(int nStrata, int nGroups, int *&idx, float *&idxScores) { return false; }
protected:


};




class DistSample : public UncertainSample
{
public:
	
					DistSample(Classifier *classify, MData *dataset);
	virtual			~DistSample(void);

	virtual bool	SelectBatch(int count, int *&ids, float *&selScores);
};



class UncertRandomSample : public UncertainSample
{
public:

				UncertRandomSample(Classifier *classify, MData *dataset);
	virtual		~UncertRandomSample(void);


	virtual bool	SelectBatch(int count, int *&ids, float *&selScores);

protected:

	float	**m_sampledSet;
	int		m_sampleSize;
	int		*m_sampleIndex;


	bool	UpdateCheckSet(void);
};



#endif
