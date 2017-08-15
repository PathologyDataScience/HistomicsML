//
//	Copyright (c) 2014-2017, Emory University
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
#if !defined(_LEARNER_H_)
#define _LEARNER_H_

#include <vector>
#include <string>
#include <set>
#include <jansson.h>

#include "data.h"
#include "ocvsvm.h"
#include "ocvrandforest.h"
#include "sampler.h"
#include "sessionClient.h"





struct SlideStat{
	string 	slide;
	double	uncertMin;
	double	uncertMax;
	float 	uncertMedian;
	double	classMin;
	double	classMax;
	int		alphaIndex;
	int		width;
	int		height;
};





class Learner : public SessionClient
{
public:
			Learner(string dataPath = "./", string outPath = "./", string heatmapPath = "./");
			~Learner(void);

	virtual bool	ParseCommand(const int sock, const char *data, int size);
	virtual bool	AutoSave(void);


protected:

	MData	*m_dataset;
	MData	*m_classTrain;	// Used for applying classifier when no session active
	string	m_dataPath;
	string 	m_outPath;
	string	m_heatmapPath;

	string	m_classifierName;
	string	m_curDatasetName;
	vector<string> m_classNames;

	vector<int> m_samples;
	vector<int>	m_curSet;
	vector<float> m_curScores;

	//set<int> m_ignoreSet; // Contains the dataset index of objects to ignore
	vector<int> m_ignoreIdx;
	vector<int>	m_ignoreId;
	vector<int> m_ignoreLabel;
	vector<int> m_ignoreIter;
	vector<string> m_ignoreSlide;

	// Training set info
	//
	int			*m_labels;
	int			*m_ids;
	float		**m_trainSet;
	int			*m_sampleIter;	// Iteration sample was added
	int			*m_slideIdx;
	float		*m_xCentroid;
	float		*m_yCentroid;
	float		*m_xClick;
	float		*m_yClick;

	int			m_iteration;
	bool		m_heatmapReload;
	vector<SlideStat*> 	m_statList;
	float		*m_scores;				// Used for heatmap generation

	float		m_curAccuracy;

	Classifier 	*m_classifier;
	Sampler		*m_sampler;

	bool		m_classifierMode;
	bool		m_debugStarted;
	bool		m_reloaded;

	bool	StartSession(const int sock, json_t *obj);
	bool	Select(const int sock, json_t *obj);
	bool	Submit(const int sock, json_t *obj);
	bool	CancelSession(const int sock, json_t *obj);
	bool	FinalizeSession(const int sock, json_t *obj);
	bool	ApplyClassifier(const int sock, json_t *obj);
	bool	Visualize(const int sock, json_t *obj);
	bool	GenHeatmap(const int sock, json_t *obj);
	bool	GenAllHeatmaps(const int sock, json_t *obj);
	bool	ReloadSession(const int sock, json_t *obj);

	bool	ApplyGeneralClassifier(const int sock, int xMin, int xMax,
								   int yMin, int yMax, string slide);
	bool	ApplySessionClassifier(const int sock, int xMin, int xMax,
								   int yMin, int yMax, string slide);
	bool	SendClassifyResult(int xMin, int xMax, int yMin, int yMax,
			 	 	 	 	   string slide, int *results, const int sock);

	bool	InitViewerClassify(const int sock, json_t *obj);

	bool 	UpdateBuffers(int updateSize, bool includeClick = false);
	void	Cleanup(void);

	bool	SaveTrainingSet(string filename);

	bool	InitSampler(void);

	float	CalcAccuracy(void);
	bool	CreateSet(vector<int> folds, int fold, float *&trainX,
					  int *&trainY, float *&testX, int *&testY);

	bool	LoadDataset(string dataSetFileName);
	bool	LoadTrainingSet(string trainingSetName);

	void	HeatmapWorker(float *slideScores, float *centX, float *centY, int numObjs,
						  string slide, int width, int height, double *uncertMin, double *uncertMax,
						  double *classMin, double *classMax, float *uncertMedian);

	bool	Review(const int sock, json_t *obj);
	bool	SaveReview(const int sock, json_t *obj);

	bool	RemoveIgnored(void);

	bool	RestoreSessionData(MData &trainingSet);

	bool	GenHeatmapSRegion(const int sock, json_t *obj);
	bool	GenAllHeatmapsSRegion(const int sock, json_t *obj);
	void	HeatmapWorkerSRegion(float *slideScores, float *centX, float *centY, int numObjs,
						  string slide, int width, int height, double *uncertMin, double *uncertMax,
						  double *classMin, double *classMax, float *uncertMedian);

};




#endif
