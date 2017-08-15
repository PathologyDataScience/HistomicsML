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
#if !defined(PICKER_H_)
#define PICKER_H_

#include <vector>
#include <string>
#include <set>
#include <jansson.h>

#include "data.h"
#include "ocvsvm.h"
#include "ocvrandforest.h"
#include "sampler.h"
#include "sessionClient.h"






class Picker : public SessionClient
{

public:

	Picker(string dataPath = "./", string outPath = "./", string heatmapPath = "./");
	~Picker(void);

	virtual bool	ParseCommand(const int sock, const char *data, int size);


protected:

	MData	*m_dataset;

	MData	*m_classTrain;	// Used for applying classifier when no session active

	string	m_dataPath;
	string 	m_outPath;

	string	m_testsetName;
	string	m_curDatasetName;
	vector<string> m_classNames;

	vector<int> m_samples;

	// Training set info
	//
	int		*m_labels;
	int		*m_ids;
	float	**m_trainSet;
	int		*m_slideIdx;
	float	*m_xCentroid;
	float	*m_yCentroid;
	float	*m_xClick;
	float	*m_yClick;

	bool	m_reloaded;

	bool	CancelSession(const int sock, json_t *obj);
	bool	GetSelected(const int sock, json_t *obj);
	bool	InitPicker(const int sock, json_t *obj);
	bool	ReloadPicker(const int sock, json_t *obj);
	bool	AddObjects(const int sock, json_t *obj);
	bool	PickerStatus(const int sock, json_t *obj);
	bool	PickerFinalize(const int sock, json_t *obj);

	bool 	RestoreSessionData(MData& testSet);

	bool	SendSelected(int xMin, int xMax, int yMin, int yMax,
			 	 	 	 string slide, int *results, const int sock);


	bool 	UpdateBuffers(int updateSize, bool includeClick = false);
	void	Cleanup(void);

	bool	SaveTrainingSet(string filename);

	bool	LoadDataset(string dataSetFileName);

	bool 	PickerReview(const int sock, json_t *obj);
	bool	PickerReviewSave(const int sock, json_t *obj);

	bool	RemoveDuplicated(void);
	bool	RemoveIgnored(void);
};


#endif /* PICKER_H_ */
