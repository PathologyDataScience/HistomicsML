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
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <jansson.h>
#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <thread>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/contrib/contrib.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "learner.h"
#include "commands.h"
#include "logger.h"



using namespace std;

const char passResp[] = "PASS";
const char failResp[] = "FAIL";


// Number of objects to sample per iteration
#define SAMPLE_OBJS 	8


extern EvtLogger *gLogger;


Learner::Learner(string dataPath, string outPath, string heatmapPath) :
m_dataset(NULL),
m_classTrain(NULL),
m_labels(NULL),
m_ids(NULL),
m_trainSet(NULL),
m_classifier(NULL),
m_sampler(NULL),
m_dataPath(dataPath),
m_outPath(outPath),
m_heatmapPath(heatmapPath),
m_sampleIter(NULL),
m_slideIdx(NULL),
m_curAccuracy(0.0f),
m_heatmapReload(false),
m_xCentroid(NULL),
m_yCentroid(NULL),
m_debugStarted(false),
m_scores(NULL),
m_classifierMode(false),
m_xClick(NULL),
m_yClick(NULL),
m_reloaded(false)
{
	memset(m_UID, 0, UID_LENGTH + 1);
	m_samples.clear();

	srand(time(NULL));
}





Learner::~Learner(void)
{
	Cleanup();
}





//-----------------------------------------------------------------------------



//
// Save the training set in its current state. This is meant to be
// initiated by the session manager.
//
bool Learner::AutoSave(void)
{
	bool	result = true;
	string 	fileName = m_classifierName + ".h5", fqfn;

	fqfn = m_outPath + fileName;
	struct stat buffer;
	if( stat(fqfn.c_str(), &buffer) == 0 ) {
		string 	tag = &m_UID[UID_LENGTH - 3];

		fileName = m_classifierName + "_" + tag + ".h5";
	} else {

	}

	if( result ) {
		result = SaveTrainingSet(fileName);
	}

	return result;
}





void Learner::Cleanup(void)
{

	m_samples.clear();
	m_classNames.clear();

	m_ignoreIdx.clear();
	m_ignoreId.clear();
	m_ignoreLabel.clear();
	m_ignoreIter.clear();
	m_ignoreSlide.clear();

	if( m_dataset ) {
		delete m_dataset;
		m_dataset = NULL;
	}

	if( m_classifier ) {
		delete m_classifier;
		m_classifier = NULL;
	}

	if( m_sampler ) {
		delete m_sampler;
		m_sampler = NULL;
	}

	if( m_labels ) {
		free(m_labels);
		m_labels = NULL;
	}

	if( m_ids ) {
		free(m_ids);
		m_ids = NULL;
	}

	if( m_trainSet ) {
		free(m_trainSet);
		m_trainSet = NULL;
	}

	if( m_sampleIter ) {
		free(m_sampleIter);
		m_sampleIter = NULL;
	}

	if( m_xCentroid ) {
		free(m_xCentroid);
		m_xCentroid = NULL;
	}

	if( m_yCentroid ) {
		free(m_yCentroid);
		m_yCentroid = NULL;
	}

	if( m_xClick ) {
		free(m_xClick);
		m_xClick = NULL;
	}

	if( m_yClick ) {
		free(m_yClick);
		m_yClick = NULL;
	}

	if( m_scores ) {
		free(m_scores);
		m_scores = NULL;
	}
	m_debugStarted = false;
}






bool Learner::ParseCommand(const int sock, const char *data, int size)
{
	bool		result = true;
	json_t		*root;
	json_error_t error;

	root = json_loads(data, 0, &error);
	if( !root ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Error parsing json");
		result = false;
	}

	if( result ) {
		json_t	*cmdObj = json_object_get(root, "command");

		if( !json_is_string(cmdObj) ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Command is not a string");
			result = false;
		} else {
			const char	*command = json_string_value(cmdObj);

			gLogger->LogMsg(EvtLogger::Evt_INFO, "Processing: %s", command);

			// 	Be careful with command names that can be a prefix of another.
			// 	i.e. init can be a prefix of initPicker. If you want to do this
			//	check for the prefix only version ("init" in the previous example)
			//	before the others.
			//
			if( strncmp(command, CMD_INIT, strlen(CMD_INIT)) == 0 ) {
				result = StartSession(sock, root);
			} else if( strncmp(command, CMD_RELOAD, strlen(CMD_RELOAD)) == 0 ) {
				result = ReloadSession(sock, root);
			} else if( strncmp(command, CMD_PRIME, strlen(CMD_PRIME)) == 0 ) {
				result = Submit(sock, root);
			}else if( strncmp(command, CMD_SELECT, strlen(CMD_SELECT)) == 0 ) {
				result = Select(sock, root);
			} else if( strncmp(command, CMD_END, strlen(CMD_END)) == 0 ) {
				result = CancelSession(sock, root);
			} else if( strncmp(command, CMD_SUBMIT, strlen(CMD_SUBMIT)) == 0 ) {
				result = Submit(sock, root);
			} else if( strncmp(command, CMD_FINAL, strlen(CMD_FINAL)) == 0 ) {
				result = FinalizeSession(sock, root);
			} else if( strncmp(command, CMD_APPLY, strlen(CMD_APPLY)) == 0 ) {
				result = ApplyClassifier(sock, root);
			} else if( strncmp(command, CMD_VISUAL, strlen(CMD_VISUAL)) == 0 ) {
				result = Visualize(sock, root);
			} else if( strncmp(command, CMD_VIEWLOAD, strlen(CMD_VIEWLOAD)) == 0) {
				result = InitViewerClassify(sock, root);
			} else if( strncmp(command, CMD_HEATMAP, strlen(CMD_HEATMAP)) == 0) {
				result = GenHeatmap(sock, root);
			} else if( strncmp(command, CMD_ALLHEATMAPS, strlen(CMD_ALLHEATMAPS)) == 0) {
				result = GenAllHeatmaps(sock, root);
			} else if( strncmp(command, CMD_SREGIONHEATMAP, strlen(CMD_SREGIONHEATMAP)) == 0) {
				result = GenHeatmapSRegion(sock, root);
			} else if( strncmp(command, CMD_SREGIONALLHEATMAPS, strlen(CMD_SREGIONALLHEATMAPS)) == 0) {
				result = GenAllHeatmapsSRegion(sock, root);

			} else if( strncmp(command, CMD_CLASSEND, strlen(CMD_CLASSEND)) == 0 ) {
				// Do nothing
				result = true;
			} else if( strncmp(command, CMD_REVIEWSAVE, strlen(CMD_REVIEWSAVE)) == 0 ) {
				result = SaveReview(sock, root);
			} else if( strncmp(command, CMD_REVIEW, strlen(CMD_REVIEW)) == 0 ) {
				result = Review(sock, root);
			} else {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Invalid command");
				result = false;
			}
		}
		json_decref(root);
	}
	return result;
}







bool Learner::StartSession(const int sock, json_t *obj)
{
	bool	result = true, uidUpdated = false;
	json_t	*jsonObj;
	const char *fileName, *uid, *classifierName;

	// m_UID's length is 1 greater than UID_LENGTH, So we can
	// always write a 0 there to make strlen safe.
	//
	m_UID[UID_LENGTH] = 0;

	if( strlen(m_UID) > 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Session already in progress: %s", m_UID);
 		result = false;
	}

	if( result ) {
		m_iteration = 0;
		jsonObj = json_object_get(obj, "features");
		fileName = json_string_value(jsonObj);
		if( fileName == NULL ) {
			result = false;
		}
	}

	if( result ) {
		jsonObj = json_object_get(obj, "uid");
		uid = json_string_value(jsonObj);
		if( uid == NULL ) {
			result = false;
		}
	}

	if( result ) {
		strncpy(m_UID, uid, UID_LENGTH);
		uidUpdated = true;
		m_dataset = new MData();
		if( m_dataset == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create dataset object");
			result = false;
		}
	}

	if( result ) {
		jsonObj = json_object_get(obj, "name");
		classifierName = json_string_value(jsonObj);
		if( classifierName != NULL ) {
			m_classifierName = classifierName;
		} else {
			result = false;
		}
	}

	if( result ) {
		jsonObj = json_object_get(obj, "negClass");
		classifierName = json_string_value(jsonObj);
		if( classifierName != NULL ) {
			m_classNames.push_back(classifierName);
		} else {
			result = false;
		}
	}

	if( result ) {
		jsonObj = json_object_get(obj, "posClass");
		classifierName = json_string_value(jsonObj);
		if( classifierName != NULL ) {
			m_classNames.push_back(classifierName);
		} else {
			result = false;
		}
	}

	if( result ) {
		string fqFileName = m_dataPath + string(fileName);

		gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading %s", fqFileName.c_str());
		double	start = gLogger->WallTime();
		result = m_dataset->Load(fqFileName);
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading took %f", gLogger->WallTime() - start);

	}

	// Create classifier and sampling objects
	//
	if( result ) {
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Loaded... %d objects of %d dimensions",
							m_dataset->GetNumObjs(), m_dataset->GetDims());

 		m_classifier = new OCVBinaryRF();

		if( m_classifier == NULL ) {
			result = false;
		}
	}

	if( result ) {
		m_sampler = new UncertainSample(m_classifier, m_dataset);
		if( m_sampler == NULL ) {
			result = false;
		}
	}

	if( result ) {
		// Allocate buffer for object scores to calculate heatmaps
		m_scores = (float*)malloc(m_dataset->GetNumObjs() * sizeof(float));
		if( m_scores == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to allocate buffer for object scores");
			result = false;
		}
	}

	// Send result back to client
	//
	size_t bytesWritten = ::write(sock, (result) ? passResp : failResp ,
								((result) ? sizeof(passResp) : sizeof(failResp)) - 1);

	if( bytesWritten != sizeof(failResp) - 1 )
		result = false;

	if( !result && uidUpdated ){
		// Initialization failed, clear current UID
		memset(m_UID, 0, UID_LENGTH + 1);
	}
	return result;
}






bool Learner::ReloadSession(const int sock, json_t *obj)
{
	bool	result = true, uidUpdated = false;
	json_t	*jsonObj;
	const char *featureFileName, *uid, *classifierName, *trainingFileName;

	// m_UID's length is 1 greater than UID_LENGTH, So we can
	// always write a 0 there to make strlen safe.
	//
	m_UID[UID_LENGTH] = 0;

	if( strlen(m_UID) > 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Session already in progress: %s", m_UID);
 		result = false;
	}

	if( result ) {
		m_iteration = 0;
		jsonObj = json_object_get(obj, "features");
		featureFileName = json_string_value(jsonObj);
		if( featureFileName == NULL ) {
			result = false;
		}
	}

	if( result ) {
		m_iteration = 0;
		jsonObj = json_object_get(obj, "trainingfile");
		trainingFileName = json_string_value(jsonObj);
		if( trainingFileName == NULL ) {
			result = false;
		}
	}

	if( result ) {
		jsonObj = json_object_get(obj, "uid");
		uid = json_string_value(jsonObj);
		if( uid == NULL ) {
			result = false;
		}
	}

	if( result ) {
		jsonObj = json_object_get(obj, "trainingset");
		classifierName = json_string_value(jsonObj);
		if( classifierName != NULL ) {
			m_classifierName = classifierName;
		} else {
			result = false;
		}
	}

	if( result ) {
		strncpy(m_UID, uid, UID_LENGTH);
		uidUpdated = true;
		m_dataset = new MData();
		if( m_dataset == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create dataset object");
			result = false;
		}
	}

	MData  trainingData;

	if( result ) {

		string fqn = m_outPath + trainingFileName;

		if( trainingData.Load(fqn) == false ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to load trining set");
			result = false;
		}
	}

	if( result ) {
		string fqFileName = m_dataPath + string(featureFileName);

		gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading %s", fqFileName.c_str());
		double	start = gLogger->WallTime();
		result = m_dataset->Load(fqFileName);
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading took %f", gLogger->WallTime() - start);
	}

	if( result ) {
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Loaded dataset... %d objects of %d dimensions",
							m_dataset->GetNumObjs(), m_dataset->GetDims());

		result = RestoreSessionData(trainingData);
	}

	// Create classifier and sampling objects
	//
	if( result ) {
		gLogger->LogMsg(EvtLogger::Evt_INFO, "ReLoaded trainingset %s, %d objects",
							trainingFileName, trainingData.GetNumObjs());

 		m_classifier = new OCVBinaryRF();

		if( m_classifier == NULL ) {
			result = false;
		}
	}

	if( result ) {
		m_sampler = new UncertainSample(m_classifier, m_dataset);
		if( m_sampler == NULL ) {
			result = false;
		}
	}

	if( result ) {
		// Allocate buffer for object scores to calculate heatmaps
		m_scores = (float*)malloc(m_dataset->GetNumObjs() * sizeof(float));
		if( m_scores == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to allocate buffer for object scores");
			result = false;
		}
	}

	if( result ) {

		int	*ptr = &m_samples[0];
		result  = m_sampler->Init(m_samples.size(), ptr);

		if( result ) {
			result = m_classifier->Train(m_trainSet[0], m_labels, m_samples.size(), m_dataset->GetDims());
		}

		if( result ) {
			m_curAccuracy = 0.0; //CalcAccuracy();

			// Classify all objects for heatmap generation
			//
			float 	**ptr = m_dataset->GetData();
			int		dims = m_dataset->GetDims();
			double start = gLogger->WallTime();

			result = m_classifier->ScoreBatch(ptr, m_dataset->GetNumObjs(), dims, m_scores);
			if( result == false ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Classification failed");
			}
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Dataset classification took %f", gLogger->WallTime() - start);
		}

		// Make sure current set is clear.
		m_curSet.clear();
	}


	// Send result back to client
	//
	json_t 	*root = json_object(), *value = NULL;
	size_t 	bytesWritten;

	if( root != NULL ) {
		json_object_set(root, "negName", json_string(m_classNames[0].c_str()));
		json_object_set(root, "posName", json_string(m_classNames[1].c_str()));
		json_object_set(root, "iteration", json_integer(m_iteration));
		json_object_set(root, "result", json_string("PASS"));

		char *jsonObj = json_dumps(root, 0);
		bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(root);
		free(jsonObj);

		// Everything updated, continue on with next iteration.
		m_iteration++;
		m_heatmapReload = true;

	} else {
		result = false;
		bytesWritten = ::write(sock, failResp, sizeof(failResp) - 1);

		if( bytesWritten != sizeof(failResp) - 1 )
			result = false;
	}

	return result;
}





bool Learner::RestoreSessionData(MData &trainingSet)
{
	bool result = true;

	int numObjs = trainingSet.GetNumObjs(),
		numDims = trainingSet.GetDims();

	result = UpdateBuffers(numObjs, true);
	if( result ) {
		float	*floatData = NULL;
		int		*intData = NULL;

		intData = trainingSet.GetIterationList();
		memcpy(m_sampleIter, intData, numObjs * sizeof(int));

		intData = trainingSet.GetLabels();
		memcpy(m_labels, intData, numObjs * sizeof(int));

		intData = trainingSet.GetIdList();
		memcpy(m_ids, intData, numObjs * sizeof(int));

		floatData = trainingSet.GetXCentroidList();
		memcpy(m_xCentroid, floatData, numObjs * sizeof(float));

		floatData = trainingSet.GetXClickList();
		// Training sets created with earlier versions of HistomicsML don't have clicks
		if( floatData == NULL ) {
			// Use centroids for click location if not present.
			floatData = trainingSet.GetXCentroidList();
		}
		memcpy(m_xClick, floatData, numObjs * sizeof(float));

		floatData = trainingSet.GetYCentroidList();
		memcpy(m_yCentroid, floatData, numObjs * sizeof(float));

		floatData = trainingSet.GetYClickList();
		if( floatData == NULL ) {
			floatData = trainingSet.GetYCentroidList();
		}
		memcpy(m_yClick, floatData, numObjs * sizeof(float));

		floatData = trainingSet.GetData()[0];
		memcpy(m_trainSet[0], floatData, numObjs * numDims * sizeof(float));

		char **classNames = trainingSet.GetClassNames();
		// Older versions of al_server did not save class names, set
		// defaults if they can't be loaded.
		if( classNames ) {
			for(int i = 0; i < trainingSet.GetNumClasses(); i++) {
				m_classNames.push_back(string(classNames[i]));
			}
		} else {
			m_classNames.push_back(string("Negative"));
			m_classNames.push_back(string("Positive"));
		}

		// Get slide indices from the dataset, NOT from the training set.
		intData = trainingSet.GetSlideIndices();
		char **slideNames = trainingSet.GetSlideNames();
		int idx;

		m_iteration = m_sampleIter[0];

		for(int i = 0; i < numObjs; i++) {

			m_slideIdx[i] = m_dataset->GetSlideIdx(slideNames[intData[i]]);

			// Keep track fo selected items
			idx = m_dataset->FindItem(m_xCentroid[i], m_yCentroid[i], slideNames[intData[i]]);
			if( idx == -1 ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to find item in dataset");
				result = false;
				break;
			} else {
				m_samples.push_back(idx);
			}

			// Find last iteration
			if( m_sampleIter[i] > m_iteration )
				m_iteration = m_sampleIter[i];
		}
	}

	return true;
}





//
// Review samples selected by user
//
//
bool Learner::Review(const int sock, json_t *obj)
{
	bool	result = true;
	json_t 	*root = NULL, *value = NULL, *sample = NULL, *sampleArray = NULL;
	int		reqIteration, idx;
	float	score;

	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);
	result = IsUIDValid(uid);

	if( result ) {
		root = json_object();

		if( root == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR,  "(select) Error creating JSON object");
			result = false;
		}
	}

	if( result ) {
		sampleArray = json_array();
		if( sampleArray == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_INFO, "(select) Unable to create sample JSON Array");
			result = false;
		}
	}

	if( result ) {

		for(int i = 0; i < m_samples.size(); i++) {

			sample = json_object();
			if( sample == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create sample JSON object");
				result = false;
				break;
			}

			int idx = m_samples[i];
			json_object_set(sample, "id", json_integer(m_ids[i]));
			json_object_set(sample, "label", json_integer(m_labels[i]));
			json_object_set(sample, "iteration", json_integer(m_sampleIter[i]));
			json_object_set(sample, "slide", json_string(m_dataset->GetSlide(idx)));
			json_object_set(sample, "centX", json_real(m_dataset->GetXCentroid(idx)));
			json_object_set(sample, "centY", json_real(m_dataset->GetYCentroid(idx)));
			json_object_set(sample, "boundary", json_string(""));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

		for(int i = 0; i < m_ignoreIdx.size(); i++) {

			sample = json_object();
			if( sample == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create sample JSON object");
				result = false;
				break;
			}

			int idx = m_ignoreIdx[i];
			int id = m_ignoreId[i];
			int label = m_ignoreLabel[i];
			int iter = m_ignoreIter[i];

			json_object_set(sample, "id", json_integer(id));
			json_object_set(sample, "label", json_integer(label));
			json_object_set(sample, "iteration", json_integer(iter));
			json_object_set(sample, "slide", json_string(m_dataset->GetSlide(idx)));
			json_object_set(sample, "centX", json_real(m_dataset->GetXCentroid(idx)));
			json_object_set(sample, "centY", json_real(m_dataset->GetYCentroid(idx)));
			json_object_set(sample, "boundary", json_string(""));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

	}

	if( result ) {
		json_object_set(root, "review", sampleArray);
		json_decref(sampleArray);

		char *jsonStr = json_dumps(root, 0);
		size_t bytesWritten = ::write(sock, jsonStr, strlen(jsonStr));

		if( bytesWritten != strlen(jsonStr) )
			result = false;
		free(jsonStr);
	}
	json_decref(root);

	return result;
}


//
// save labels from review
//
//
bool Learner::SaveReview(const int sock, json_t *obj)
{
	bool	result = true;
	json_t 	*jsonObj = NULL, *value = NULL, *sampleArray = NULL;
	int count = 0;
	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);
	result = IsUIDValid(uid);

	if( result ) {
		sampleArray = json_object_get(obj, "samples");
		if( !json_is_array(sampleArray) ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Invalid samples array");
			result = false;
		}
	}

	if( result ) {
		size_t	index;
		int id, label, sample_id;

		json_array_foreach(sampleArray, index, jsonObj) {

			value = json_object_get(jsonObj, "id");
			id = json_integer_value(value);

			value = json_object_get(jsonObj, "label");
			label = json_integer_value(value);

			// Look for the sample to update.
			// in the case of m_samples only
			for(int i = 0; i < m_samples.size(); i++) {
				if( id == m_ids[i] ) {
					count++;
					m_labels[i] = label;
				}
			}

			// in the case of m_ignores only
			for(int i = 0; i < m_ignoreIdx.size(); i++) {
				int igr_id = m_ignoreId[i];
				if( id == igr_id ) {
					count++;
					m_ignoreLabel[i] = label;
				}
			}

		}
	}

	// Send result back to client
	//
	json_t 	*root = json_object();
	size_t 	bytesWritten;

	if( root != NULL ) {
		if( result ) {
			json_object_set(root, "status", json_string("PASS"));
			json_object_set(root, "updated", json_integer(count));
		} else {
			json_object_set(root, "status", json_string("FAIL"));
		}

		char *jsonObj = json_dumps(root, 0);
		bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(root);
		free(jsonObj);

	}

	return result;
}

bool Learner::RemoveIgnored(void)
{
	bool 	result = true;
	int 	newLength = m_samples.size(), i = 0;

	while( i < newLength ) {

		if( m_labels[i] == 0 ) {

			int idx;
		 	char *slide = m_dataset->GetSlide(m_slideIdx[i]);
			idx = m_dataset->FindItem(m_xCentroid[i], m_yCentroid[i], slide);

			m_ignoreIdx.push_back(idx);
			m_ignoreId.push_back(m_ids[i]);
			m_ignoreLabel.push_back(m_labels[i]);
			m_ignoreIter.push_back(m_sampleIter[i]);
			m_ignoreSlide.push_back(slide);

			newLength--;
			while( m_labels[newLength] == 0 && newLength > i ) {
				// Make sure the sample we are swapping is not to be
				// ignored
				newLength--;
			}

			// No need to swap if last samples
			if( i < newLength ) {
				m_labels[i] = m_labels[newLength];
				m_ids[i] = m_ids[newLength];
				m_sampleIter[i] = m_sampleIter[newLength];
				memcpy(m_trainSet[i], m_trainSet[newLength], m_dataset->GetDims());
				m_slideIdx[i] = m_slideIdx[newLength];
				m_xCentroid[i] = m_xCentroid[newLength];
				m_yCentroid[i] = m_yCentroid[newLength];
			}
		}

		i++;
	}

	// Remove the extra samples from the end of the vector, the Finalize code uses
	// the sample vector's length for the number of samples to save.
	int diff = m_samples.size() - newLength;

	m_samples.erase(m_samples.end()-(diff+1), m_samples.end()-1);

	return result;
}


//
// Select new samples for the user to label.
//
//
bool Learner::Select(const int sock, json_t *obj)
{
	bool	result = true;
	json_t 	*root = NULL, *value = NULL, *sample = NULL, *sampleArray = NULL;
	int		reqIteration, idx;
	float	score;

	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);
	result = IsUIDValid(uid);

	if( result ) {
		value = json_object_get(obj, "iteration");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(select) Unable to decode iteration");
			result = false;
		} else {
			reqIteration = json_integer_value(value);
		}
	}

	if( result ) {
		root = json_object();

		if( root == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR,  "(select) Error creating JSON object");
			result = false;
		}
	}

	if( result ) {
		sampleArray = json_array();
		if( sampleArray == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_INFO, "(select) Unable to create sample JSON Array");
			result = false;
		}
	}

	if( result ) {

		json_object_set(root, "iteration", json_integer(m_iteration));
		json_object_set(root, "accuracy", json_real(m_curAccuracy));

		int 	*selIdx = NULL;
		float	*selScores = NULL;

		if( m_iteration != reqIteration ) {
			double	start = gLogger->WallTime();
			// Get new samples
			m_sampler->SelectBatch(SAMPLE_OBJS, selIdx, selScores);
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Select took %f", gLogger->WallTime() - start);
		}

		for(int i = 0; i < SAMPLE_OBJS; i++) {

			sample = json_object();
			if( sample == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create sample JSON object");
				result = false;
				break;
			}

			if( m_iteration != reqIteration ) {
				// Get new sample
				idx = selIdx[i];
				m_curSet.push_back(idx);
				score = selScores[i];
				m_curScores.push_back(selScores[i]);
			} else {
				// Haven't submitted that last group of selections. Send
				// them again
				score = m_curScores[i];
				idx = m_curSet[i];
			}

			json_object_set(sample, "slide", json_string(m_dataset->GetSlide(idx)));
			json_object_set(sample, "id", json_integer(0));
			json_object_set(sample, "centX", json_real(m_dataset->GetXCentroid(idx)));
			json_object_set(sample, "centY", json_real(m_dataset->GetYCentroid(idx)));
			json_object_set(sample, "label", json_integer((score < 0) ? -1 : 1));
			json_object_set(sample, "score", json_real(score));
			json_object_set(sample, "maxX", json_integer(0));
			json_object_set(sample, "maxY", json_integer(0));
			json_object_set(sample, "boundary", json_string(""));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}
		if( selIdx )
			free(selIdx);
		if( selScores )
			free(selScores);
	}

	if( result ) {
		json_object_set(root, "samples", sampleArray);
		json_decref(sampleArray);

		char *jsonObj = json_dumps(root, 0);
		size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(root);
		free(jsonObj);
	}

	return result;
}






//
//	User has indicated the label for each of the previously
//	selected samples. Add them to the training set.
//
bool Learner::Submit(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*sampleArray, *value, *jsonObj;
	int		iter;

	// Check for valid UID
	//
	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);
	result = IsUIDValid(uid);

	// Sanity check the itereation
	//
	if( result ) {
		value = json_object_get(obj, "iteration");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(submit) Unable to decode iteration");
			result = false;
		} else {
			iter = json_integer_value(value);
			// Check iteration to make sure we're in sync
			//
			if( iter == -1 ) {
				// "fixed" samples being added by hand. Indicate with the negation of
				// the current iteration.
				iter = -m_iteration;
			} else if( iter != m_iteration ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Resubmitting not allowed");;
				result = false;
			}
		}
	}

	if( result ) {
		sampleArray = json_object_get(obj, "samples");
		if( !json_is_array(sampleArray) ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Invalid samples array");
			result = false;
		}
	}

	if( result ) {
		size_t	index;
		int		id, label, idx, dims = m_dataset->GetDims();
		float	centX, centY;
		const char *slide;
		double	start = gLogger->WallTime();

		json_array_foreach(sampleArray, index, jsonObj) {

			value = json_object_get(jsonObj, "id");
			id = json_integer_value(value);

			value = json_object_get(jsonObj, "label");
			label = json_integer_value(value);

			// The dynamic typing in PHP or javascript can make a float
			// an int if it has no decimal portion. Since the centroids can
			// be whole numbers, we need to check if they are real, if not
			// we just assume they are integer
			//
			value = json_object_get(jsonObj, "centX");
			if( json_is_real(value) )
				centX = (float)json_real_value(value);
			else
				centX = (float)json_integer_value(value);

			value = json_object_get(jsonObj, "centY");
			if( json_is_real(value) )
				centY = (float)json_real_value(value);
			else
				centY = (float)json_integer_value(value);

			//
			// Slide names that are numbers may have been converted to an integer.
			// Try decoding a string first, if that fails, try an integer
			//
			value = json_object_get(jsonObj, "slide");
			if( json_is_string(value) )
				slide = json_string_value(value);
			else {
				char name[255];
				int num = json_integer_value(value);
				snprintf(name, 255, "%d", num);
				slide = name;
			}

			// Get the dataset index for this object
			idx = m_dataset->FindItem(centX, centY, slide);


			if( label == 0 ) {
				//m_ignoreSet.insert(idx);
				m_ignoreIdx.push_back(idx);
				m_ignoreId.push_back(id);
				m_ignoreLabel.push_back(0);
				m_ignoreIter.push_back(iter);
				m_ignoreSlide.push_back(slide);

			} else {

				result = UpdateBuffers(1);

				if( result ) {

					int	pos = m_samples.size();

					if( idx != -1 ) {
						m_labels[pos] = label;
						m_ids[pos] = id;
						m_sampleIter[pos] = iter;
						m_slideIdx[pos] = m_dataset->GetSlideIdx(slide);
						m_xCentroid[pos] = m_dataset->GetXCentroid(idx);
						m_yCentroid[pos] = m_dataset->GetYCentroid(idx);
						result = m_dataset->GetSample(idx, m_trainSet[pos]);
						m_samples.push_back(idx);
					} else {
						gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to find item: %s, %f, %f ", slide, centX, centY);
						result = false;
					}
				}
			}

			// Something is wrong, stop processing
			if( !result )
				break;
		}
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Submit took %f, for %d samples",
						gLogger->WallTime() - start, json_array_size(sampleArray));

		//
		// Indicate training set has been updated and heatmaps need to be rebuilt
		m_heatmapReload = true;
	}

	// update the training set if the label is not zero in ignores
	if( result ) {
		for(int i = 0; i < m_ignoreIdx.size(); i++) {

			int idx = m_ignoreIdx[i];
			int id = m_ignoreId[i];
			int label = m_ignoreLabel[i];
			int iter = m_ignoreIter[i];

			if( label != 0 ) {

				result = UpdateBuffers(1);

				if( result ) {

					int	pos = m_samples.size();

					if( idx != -1 ) {
						m_labels[pos] = label;
						m_ids[pos] = id;
						m_sampleIter[pos] = iter;
						m_slideIdx[pos] = m_dataset->GetSlideIdx(m_ignoreSlide[i].c_str());
						m_xCentroid[pos] = m_dataset->GetXCentroid(idx);
						m_yCentroid[pos] = m_dataset->GetYCentroid(idx);
						result = m_dataset->GetSample(idx, m_trainSet[pos]);
						m_samples.push_back(idx);
					} else {
						gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to find item: %d", id);
						result = false;
					}
				}
				m_ignoreIdx.erase(m_ignoreIdx.begin() + i);
				m_ignoreId.erase(m_ignoreId.begin() + i);
				m_ignoreLabel.erase(m_ignoreLabel.begin() + i);
				m_ignoreIter.erase(m_ignoreIter.begin() + i);
				m_ignoreSlide.erase(m_ignoreSlide.begin() + i);

			}
		}
	}

	// remove ignores before updating the training set
	result = RemoveIgnored();

	// If all's well, train the classifier with the updated training set
	//
	if( result ) {

		if( m_iteration == 0 ) {
			// First iteration init the sampler with the objects
			// selected by the user.
			int	*ptr = &m_samples[0];
			result  = m_sampler->Init(m_samples.size(), ptr);
		}

		double	start = gLogger->WallTime();

		// Only increment if submitted from the normal active learning process.
		if( iter >= 0 ) {
			m_iteration++;
		}
		result = m_classifier->Train(m_trainSet[0], m_labels, m_samples.size(), m_dataset->GetDims());
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Classifier training took %f", gLogger->WallTime() - start);

		// Need to select new samples.
		m_curSet.clear();
	}

	// Send result back to client
	//
	size_t bytesWritten = ::write(sock, (result) ? passResp : failResp ,
								((result) ? sizeof(passResp) : sizeof(failResp)) - 1);
	if( bytesWritten != (sizeof(failResp) - 1) )
		result = false;

	return result;
}






bool Learner::FinalizeSession(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*root = json_object(), *sample = NULL, *sampleArray = NULL,
			*jsonObj = NULL;
	int		idx;
	float	score;
	string 	fileName = m_classifierName + ".h5", fqfn;

	jsonObj = json_object_get(obj, "uid");
	const char *uid = json_string_value(jsonObj);
	result = IsUIDValid(uid);

	if( result && m_reloaded ) {
		// Only check if the file exists when creating a new training set.
		fqfn = m_outPath + fileName;
		struct stat buffer;
		if( stat(fqfn.c_str(), &buffer) == 0 ) {
			string 	tag = &m_UID[UID_LENGTH - 3];

			fileName = m_classifierName + "_" + tag + ".h5";
		}
	}

	if( result && root == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Error creating JSON array");
		result = false;
	}

	if( result ) {
		sampleArray = json_array();
		if( sampleArray == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create sample JSON Array");
			result = false;
		}
	}

	if( result ) {
		double	start = gLogger->WallTime();

		// Iteration count starts form 0.
		json_object_set(root, "iterations", json_integer(m_iteration));
		json_object_set(root, "filename", json_string(fileName.c_str()));

		// We just return an array of the nuclei database id's, label and iteration when added
		//
		for(int i = 0; i < m_samples.size(); i++) {

			sample = json_object();
			if( sample == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create sample JSON object");
				result = false;
				break;
			}

			int idx = m_samples[i];
			json_object_set(sample, "id", json_integer(m_ids[i]));
			json_object_set(sample, "label", json_integer(m_labels[i]));
			json_object_set(sample, "iteration", json_integer(m_sampleIter[i]));

			json_array_append(sampleArray, sample);
			json_decref(sample);
		}

		gLogger->LogMsg(EvtLogger::Evt_INFO, "Finalize took %f", gLogger->WallTime() - start);
	}

	if( result ) {
		json_object_set(root, "samples", sampleArray);
		json_decref(sampleArray);

		char *jsonStr = json_dumps(root, 0);
		size_t bytesWritten = ::write(sock, jsonStr, strlen(jsonStr));

		if( bytesWritten != strlen(jsonStr) )
			result = false;
		free(jsonStr);
	}
	json_decref(root);

	// Remove ignores before saving the training set
	result = RemoveIgnored();

	// Save the training set
	//
	if( result ) {
		result = SaveTrainingSet(fileName);
	}

	// This session is done, clear the UID and cleanup associated
	// data
	//
	memset(m_UID, 0, UID_LENGTH + 1);
	Cleanup();

	return result;
}





bool Learner::Visualize(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*root = json_array(), *sample = NULL, *value = NULL;
	int		strata, groups;

	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);
	result = IsUIDValid(uid);

	if( result ) {
		value = json_object_get(obj, "strata");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(visualize) Invalid strata");
			result = false;
		} else {
			strata = json_integer_value(value);
		}
	}

	if( result ) {
		value = json_object_get(obj, "groups");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(visualize) Invalid groups");
			result = false;
		} else {
			groups = json_integer_value(value);
		}
	}

	if( root == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to crate JSON array for visualization");
		result = false;
	}

	if( result ) {
		int	*sampleIdx = NULL, totalSamp;
		float *sampleScores = NULL;
		double	start = gLogger->WallTime();


		totalSamp = strata * groups * 2;
		result = m_sampler->GetVisSamples(strata, groups, sampleIdx, sampleScores);

		if( result ) {
			for(int i = 0; i < totalSamp; i++) {
				sample = json_object();

				if( sample == NULL ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create sample JSON object");
					result = false;
					break;
				}

				json_object_set(sample, "slide", json_string(m_dataset->GetSlide(sampleIdx[i])));
				json_object_set(sample, "id", json_integer(0));
				json_object_set(sample, "centX", json_real(m_dataset->GetXCentroid(sampleIdx[i])));
				json_object_set(sample, "centY", json_real(m_dataset->GetYCentroid(sampleIdx[i])));
				json_object_set(sample, "maxX", json_integer(0));
				json_object_set(sample, "maxY", json_integer(0));
				json_object_set(sample, "score", json_real(sampleScores[i]));

				json_array_append(root, sample);
				json_decref(sample);
			}

			if( sampleIdx ) {
				free(sampleIdx);
			}
			if( sampleScores ) {
				free(sampleScores);
			}
		}
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Visualize took %f", gLogger->WallTime() - start);
	}

	if( result ) {
		char *jsonObj = json_dumps(root, 0);
		size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;
		free(jsonObj);
	}
	json_decref(root);

	return result;
}






bool Learner::UpdateBuffers(int updateSize, bool includeClick)
{
	bool	result = true;
	int		*newBuff = NULL, newSize = m_samples.size() + updateSize;
	float	*floatBuff = NULL;

	newBuff = (int*)realloc(m_labels, newSize * sizeof(int));
	if( newBuff != NULL )
		m_labels = newBuff;
	else
		result = false;

	if( result ) {
		newBuff = (int*)realloc(m_ids, newSize * sizeof(int));
		if( newBuff != NULL )
			m_ids = newBuff;
		else
			result = false;
	}

	if( result ) {
		newBuff = (int*)realloc(m_sampleIter, newSize * sizeof(int));
		if( newBuff != NULL )
			m_sampleIter = newBuff;
		else
			result = false;
	}

	if( result ) {
		floatBuff = (float*)realloc(m_xCentroid, newSize * sizeof(float));
		if( floatBuff != NULL )
			m_xCentroid = floatBuff;
		else
			result = false;
	}

	if( result ) {
		floatBuff = (float*)realloc(m_yCentroid, newSize * sizeof(float));
		if( floatBuff != NULL )
			m_yCentroid = floatBuff;
		else
			result = false;
	}

	if( result ) {
		newBuff = (int*)realloc(m_slideIdx, newSize * sizeof(int));
		if( newBuff != NULL )
			m_slideIdx = newBuff;
		else
			result = false;
	}

	if( includeClick && result ) {
		floatBuff = (float*)realloc(m_xClick, newSize * sizeof(float));
		if( floatBuff != NULL )
			m_xClick = floatBuff;
		else
			result = false;

		if( result ) {
			floatBuff = (float*)realloc(m_yClick, newSize * sizeof(float));
			if( floatBuff != NULL )
				m_yClick = floatBuff;
			else
				result = false;
		}
	}

	if( result ) {
		bool  init = (m_trainSet == NULL) ? true : false;
		float **newFeatureIdx = (float**)realloc(m_trainSet, newSize * sizeof(float*));
		if( newFeatureIdx != NULL ) {
			m_trainSet = newFeatureIdx;
			if( init ) {
				m_trainSet[0] = NULL;
			}
		} else {
			result = false;
		}
	}

	if( result ) {
		int 	dims = m_dataset->GetDims();
		float *newFeatBuff = (float*)realloc(m_trainSet[0], newSize * dims * sizeof(float));
		if( newFeatBuff != NULL ) {
			m_trainSet[0] = newFeatBuff;
			for(int i = 1; i < newSize; i++) {
				m_trainSet[i] = m_trainSet[i - 1] + dims;
			}
		} else
			result = false;
	}
	return result;
}





bool Learner::CancelSession(const int sock, json_t *obj)
{
	bool	result = true;

	// Just erase the UID for now
	memset(m_UID, 0, UID_LENGTH + 1);
	gLogger->LogMsg(EvtLogger::Evt_INFO, "Session canceled");

	Cleanup();

	size_t bytesWritten = ::write(sock, (result) ? passResp : failResp ,
							((result) ? sizeof(passResp) : sizeof(failResp)) - 1);
	if( bytesWritten != sizeof(failResp) - 1 )
		result = false;

	return result;
}






bool Learner::SaveTrainingSet(string fileName)
{
	bool	result = false;;
	MData 	*trainingSet = new MData();

	if( trainingSet != NULL ) {

		result = trainingSet->Create(m_trainSet[0], m_samples.size(), m_dataset->GetDims(),
							m_labels, m_ids, m_sampleIter, m_dataset->GetMeans(), m_dataset->GetStdDevs(),
							m_xCentroid, m_yCentroid, m_dataset->GetSlideNames(), m_slideIdx,
							m_dataset->GetNumSlides(), m_classNames);
	}

	if( result ) {
		fileName = m_outPath + fileName;
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Saving training set to: %s", fileName.c_str());
		result = trainingSet->SaveAs(fileName);
	}

	if( trainingSet != NULL )
		delete trainingSet;


	return result;
}








bool Learner::ApplyClassifier(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*value;
	const char *trainSetFile = NULL, *dataSetFile = NULL;
	MData	*dataSet = NULL, *trainingSet = NULL;
	Classifier *classifier = NULL;
	int		*results = NULL, *labels, dims;
	float	*test, *train;
	const char *slideName = NULL;
	int		xMin, xMax, yMin, yMax;

	value = json_object_get(obj, "slide");
	slideName = json_string_value(value);
	value = json_object_get(obj, "xMin");
	xMin = json_integer_value(value);
	value = json_object_get(obj, "xMax");
	xMax = json_integer_value(value);
	value = json_object_get(obj, "yMin");
	yMin = json_integer_value(value);
	value = json_object_get(obj, "yMax");
	yMax = json_integer_value(value);

	if( slideName == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyClassifier) Unable to decode slide name");
		result = false;
	}
	if( xMin == 0 || xMax == 0 || yMin == 0 || yMax == 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyClassifier) Unable to decode min/max values");
		result = false;
	}

	double timing = gLogger->WallTime();
	if( result ) {
		// Session in progress, use current training set.
		value = json_object_get(obj, "uid");
		const char *uid = json_string_value(value);
		result = IsUIDValid(uid);
 	}

	if( result ) {
		// UID was valid, there's a session active
		result = ApplySessionClassifier(sock, xMin, xMax, yMin, yMax, slideName);
	} else if( strlen(m_UID) == 0 ) {
		// No session active, load specified training set.
		result = ApplyGeneralClassifier(sock, xMin, xMax, yMin, yMax, slideName);
	}

	timing = gLogger->WallTime() - timing;
	gLogger->LogMsg(EvtLogger::Evt_INFO, "Classification took: %f", timing);

	return result;
}









//
//	Applies the specified classifier to the specified dataset. Used when
//	no session is active.
//
bool Learner::ApplyGeneralClassifier(const int sock, int xMin, int xMax,
									 int yMin, int yMax, string slide)
{
	bool	result = true;
	json_t	*value;
	Classifier *classifier = new OCVBinaryRF();

	int		*predClass = NULL, *labels = NULL;


	if( classifier == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyGeneralClassifier) Unable to allocate classifier object");
		result = false;
	}

	if( result ) {
		int	 	slideObjs;
		float 	**test, **train;
		int 	dims;

		train = m_classTrain->GetData();
		labels = m_classTrain->GetLabels();
		dims = m_dataset->GetDims();
		test = m_dataset->GetSlideData(slide, slideObjs);

		predClass = (int*)malloc(slideObjs * sizeof(int));

		if( predClass == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyGeneralClassifier) Unable to allocate results buffer");
			result = false;
		} else {
			result = classifier->Train(train[0], labels, m_classTrain->GetNumObjs(), dims);
		}

		if( result ) {
			result = classifier->ClassifyBatch(test, slideObjs, dims, predClass);
		}
	}

	if( result ) {
		result = SendClassifyResult(xMin, xMax, yMin, yMax, slide, predClass, sock);
	}

	if( predClass )
		free(predClass);
	if( classifier )
		delete classifier;

	return result;
}







bool Learner::ApplySessionClassifier(const int sock, int xMin, int xMax,
									 int yMin, int yMax, string slide)
{
	bool	result = true;
	json_t	*value;
	float 	**ptr;
	int 	*labels = NULL, dims, slideObjs, offset;

	offset = m_dataset->GetSlideOffset(slide, slideObjs);
	labels = (int*)malloc(slideObjs * sizeof(int));

	if( labels == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplySessionClassifier) Unable to allocate labels buffer");
		result = false;
	}

	if( result ) {

		ptr = m_dataset->GetSlideData(slide, slideObjs);
		dims = m_dataset->GetDims();
		result = m_classifier->ClassifyBatch(ptr, slideObjs, dims, labels);
	}

	if( result ) {
		result = SendClassifyResult(xMin, xMax, yMin, yMax, slide, labels, sock);
	}

	if( labels )
		free(labels);

	return result;
}






bool Learner::SendClassifyResult(int xMin, int xMax, int yMin, int yMax,
								 string slide, int *results, const int sock)
{
	bool	result = true;
	json_t	*root = json_object();

	if( root == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(SendClassifyResult) Unable to create JSON object");
		result = false;
	}

	if( result ) {
		char	tag[25];
		int		offset, slideObjs;

		offset = m_dataset->GetSlideOffset(slide, slideObjs);
		for(int i = offset; i < offset + slideObjs; i++) {

			if( slide.compare(m_dataset->GetSlide(i)) == 0 &&
				m_dataset->GetXCentroid(i) >= xMin && m_dataset->GetXCentroid(i) <= xMax &&
				m_dataset->GetYCentroid(i) >= yMin && m_dataset->GetYCentroid(i) <= yMax ) {

				snprintf(tag, 24, "%.1f_%.1f",
						m_dataset->GetXCentroid(i), m_dataset->GetYCentroid(i));

				json_object_set(root, tag, json_integer((results[i - offset] == -1) ? 0 : 1 ));
			}
		}

		if( result ) {
			char *jsonObj = json_dumps(root, 0);

			if( jsonObj == NULL ) {
				result = false;
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(SendClassifyResult) Unable to encode JSON");
			}

			if( result ) {
				size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

				if( bytesWritten != strlen(jsonObj) ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "(SendClassifyResult) Unable to send entire JSON object");
					result = false;
				}
			}
			json_decref(root);
			free(jsonObj);
		}
	}
	return result;
}







bool Learner::InitViewerClassify(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*value;
	const char *trainSetName = NULL, *dataSetFileName = NULL;

	m_classifierMode = true;
	// viewerLoad
	value = json_object_get(obj, "dataset");
	dataSetFileName = json_string_value(value);

	value = json_object_get(obj, "trainset");
	trainSetName = json_string_value(value);

	if( dataSetFileName == NULL || trainSetName == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "(ApplyGeneralClassifier) Dataset or training set not specified");
		result = false;
	}

	if( result ) {
		result = LoadDataset(dataSetFileName);
	}

	if( result ) {
		result = LoadTrainingSet(trainSetName);
	}

	json_t	*root, *classNames, *name;

	root = json_object();
	if( root == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create class name JSON root object");
		result = false;
	}

	if( result ) {

		gLogger->LogMsg(EvtLogger::Evt_INFO, "Loaded %d class names", m_classNames.size());
		classNames = json_array();
		if( classNames == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create class name JSON Array");
			result = false;
		}
	}

	if( result ) {
		char	tag[10];

		for(int i = 0; i < m_classNames.size(); i++) {

			name = json_object();
			if( name == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create name JSON object");
				result = false;
				break;
			}
			sprintf(tag, "class_%d", i);
			json_array_append_new(classNames, json_string(m_classNames[i].c_str()));
		}

	}

	json_object_set(root, "class_names", classNames);
	json_decref(classNames);

	char *jsonObj = json_dumps(root, 0);
	size_t bytesWritten = ::write(sock, jsonObj, strlen(jsonObj));

	if( bytesWritten != strlen(jsonObj) )
		result = false;

	json_decref(root);
	free(jsonObj);

	return result;
}






bool Learner::LoadDataset(string dataSetFileName)
{
	bool result = true;

	// Load dataset only if not loaded already
	//
	if( m_curDatasetName.compare(dataSetFileName) != 0 ) {
		m_curDatasetName = dataSetFileName;

		if( m_dataset )
			delete m_dataset;

		m_dataset = new MData();
		if( m_dataset == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(LoadDataset) Unable to create dataset object");
			result = false;
		}

		if( result ) {
			string fqn = m_dataPath + dataSetFileName;
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading: %s", fqn.c_str());
			result = m_dataset->Load(fqn);
		}
	}
	return result;
}






bool Learner::LoadTrainingSet(string trainingSetName)
{
	bool	result = true;

	// Only load if not already loaded
	//
	if( m_classifierName.compare(trainingSetName) != 0 ) {
		m_classifierName = trainingSetName;

		if( m_classTrain )
			delete m_classTrain;

		m_classTrain = new MData();
		if( m_classTrain == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(LoadTrainingSet) Unable to create trainset object");
			result = false;
		}

		string 	fqn;

		if( result ) {
			fqn = m_outPath + trainingSetName;
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Loading: %s", fqn.c_str());
			result = m_classTrain->Load(fqn);
		}

		if( result ) {
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Loaded!");
		} else {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Failed loading %s", fqn.c_str());
		}

		if( result ) {
			int 	numClasses = m_classTrain->GetNumClasses();
			char	**names = m_classTrain->GetClassNames();

			gLogger->LogMsg(EvtLogger::Evt_INFO, "Num classes: %d, names: 0x%x", numClasses, names);

			for(int i = 0; i < numClasses; i++) {
				m_classNames.push_back(names[i]);
			}
		}
	}
	return result;
}





#define FOLDS	5

//
//	Calculate the in-sample accuracy using the current
//	training set.
//
float Learner::CalcAccuracy(void)
{
	float		result = -1.0f;
	Classifier	*classifier = new OCVBinaryRF();

	if( classifier ) {
		vector<int>::iterator  it;

		classifier->Train(m_trainSet[0], m_labels, m_samples.size(), m_dataset->GetDims());

		int  *results = (int*)malloc(m_samples.size() * sizeof(int));
		if( results ) {
			classifier->ClassifyBatch(m_trainSet, m_samples.size(), m_dataset->GetDims(), results);
			int  score = 0;
			for(int i = 0; i < m_samples.size(); i++) {
				if( results[i] == m_labels[i] )
					score++;
			}
			result = (float)score / (float)m_samples.size();
			free(results);
		}
		delete classifier;
	}
#if 0
	// Randomize the data the split into 5 folds
	// foldList contains the fold the corresponding data object belongs to
	vector<int> foldList;

	for(int i = 0; i < m_samples.size(); i++) {
			foldList.push_back(i % FOLDS);
	}
	random_shuffle(foldList.begin(), foldList.end());

	float *trainX = NULL, *testX = NULL;
	int	*trainY = NULL, *testY = NULL;


	for(int fold = 0; fold < FOLDS; fold++) {
		if( CreateSet(foldList, fold, trainX, trainY, testX, testY) ) {

		} else {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create training / test set for validation");
			result = false;
			break;
		}
	}
#endif
	return result;
}







bool Learner::CreateSet(vector<int> folds, int fold, float *&trainX, int *&trainY, float *&testX, int *&testY)
{
	bool	result = true;


	return result;
}





bool Learner::GenHeatmap(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*value = NULL, *root = NULL;

	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);
	int		width, height;
	double	uncertMin, uncertMax, classMin, classMax, timing = gLogger->WallTime();
	float	uncertMedian;
	string	slide, uncertFileName, classFileName;

	result = IsUIDValid(uid);

	if( result ) {
		value = json_object_get(obj, "width");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatmap) Unable to decode width");
			result = false;
		} else {
			width = json_integer_value(value);
		}
	}

	if( result ) {
		value = json_object_get(obj, "height");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatmap) Unable to decode height");
			result = false;
		} else {
			height = json_integer_value(value);
		}
	}

	if( result ) {
		value = json_object_get(obj, "slide");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatmap) Unable to decode slide name");
			result = false;
		} else {
			slide = json_string_value(value);
		}
	}

	if( result ) {

		float	*scores = NULL, *xList = m_dataset->GetXCentroidList(), *yList = m_dataset->GetYCentroidList(),
				*centX, *centY;
		int		numObjs, offset;

		offset = m_dataset->GetSlideOffset(slide, numObjs);
		centX = &xList[offset];
		centY = &yList[offset];
		scores = (float*)malloc(numObjs * sizeof(float));

		if( scores == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatImage) Unable to allocate score buffer");
			result = false;
		}

		if( result ) {

			float 	**ptr = m_dataset->GetSlideData(slide, numObjs);
			int		dims = m_dataset->GetDims();

			result = m_classifier->ScoreBatch(ptr, numObjs, dims, scores);
			if( result == false ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatImage) Classification failed");
			}
		}

		if( result ) {
			uncertFileName = slide + ".jpg";
			classFileName = slide + "_class.jpg";

			string	fqfn = m_heatmapPath + "/" + uncertFileName;
			struct stat buffer;
			int		statResp = stat(fqfn.c_str(), &buffer);

			gLogger->LogMsg(EvtLogger::Evt_INFO, "GenHeatMap Start");

			// Check if heatmap needs to be generated
			if( m_heatmapReload || statResp != 0 ) {

				HeatmapWorker(scores, centX, centY, numObjs, slide, width, height,
							  &uncertMin, &uncertMax, &classMin, &classMax, &uncertMedian);
			} else {

				vector<SlideStat*>::iterator	it;

				for(it = m_statList.begin(); it != m_statList.end(); it++) {
					if( (*it)->slide.compare(slide) == 0 ) {
						width = (*it)->width;
						height = (*it)->height;
						uncertMin = (*it)->uncertMin;
						uncertMax = (*it)->uncertMax;
						uncertMedian = (*it)->uncertMedian;
						classMin = (*it)->classMin;
						classMax = (*it)->classMax;
						break;
					}
				}
			}
		}
	}


	if( result ) {
		root = json_object();
		if( root == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatmap) Unable to create JSON object");
			result = false;
		}
	}

	if( result ) {
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Viewer heatmap generation took %f", gLogger->WallTime() - timing);

		json_object_set(root, "width", json_integer(width));
		json_object_set(root, "height", json_integer(height));
		json_object_set(root, "uncertFilename", json_string(uncertFileName.c_str()));
		json_object_set(root, "classFilename", json_string(classFileName.c_str()));
		json_object_set(root, "uncertMin", json_real(uncertMin));
		json_object_set(root, "uncertMax", json_real(uncertMax));
		json_object_set(root, "uncertMedian", json_real(uncertMedian));
		json_object_set(root, "classMin", json_real(classMin));
		json_object_set(root, "classMax", json_real(classMax));

		char *jsonObj = json_dumps(root, 0);
		size_t  bytesWritten = :: write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(root);
		free(jsonObj);
	}
	return result;
}



#define KERN_SIZE	(ceil(3.5f * 11))
#define GRID_SIZE	40
#define HIST_BINS	20
#define UNCERT_PERCENTILE	0.90f
#define SREGION_GRID_SIZE	40



static bool SortFunc(SlideStat *a, SlideStat *b)
{
	return (a->uncertMax > b->uncertMax);
}



bool Learner::GenAllHeatmaps(const int sock, json_t *obj)
{
	bool result = true;
	json_t	*value = NULL, *slideData = NULL;


	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);
	result = IsUIDValid(uid);

	if( result && m_heatmapReload ) {
		double	start, timing = gLogger->WallTime();

		if( result ) {
			slideData = json_object_get(obj, "slides");
			if( slideData == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode slide data");
				result = false;
			}
		}

		json_t	*slides = NULL, *x_size = NULL, *y_size = NULL;

		if( result ) {

			slides = json_object_get(slideData, "slides");
			if( !json_is_array(slides) ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode slide name array");
				result = false;
			}
		}

		if( result ) {

			x_size = json_object_get(slideData, "x_size");
			if( !json_is_array(x_size) ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode x_size array");
				result = false;
			}
		}

		if( result ) {

			y_size = json_object_get(slideData, "y_size");
			if( !json_is_array(y_size) ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode y_size array");
				result = false;
			}
		}

		// To identify the most recent scores,
		// Classify all objects for heatmap generation
		//
		float 	**ptr = m_dataset->GetData();
		int	dims = m_dataset->GetDims();
		result = m_classifier->ScoreBatch(ptr, m_dataset->GetNumObjs(), dims, m_scores);
		if( result == false ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Classification failed");
		}

		if( result ) {
			size_t index, numSlides = json_array_size(slides);
			const char 	*slideName = NULL;
			int			width, height;
			vector<std::thread> workers;


			if( !m_statList.empty() ) {
				for(int i = 0; i < m_statList.size(); i++) {
					delete m_statList[i];
				}
				m_statList.clear();
			}

			json_array_foreach(slides, index, value) {

				slideName = json_string_value(value);
				if( slideName == NULL ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode slide name");
					result = false;
					break;
				}

				value = json_array_get(x_size, index);
				if( value == NULL ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode width");
					result = false;
					break;
				}
				width = json_integer_value(value);

				value = json_array_get(y_size, index);
				if( value == NULL ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode height");
					result = false;
					break;
				}
				height = json_integer_value(value);

				int slideObjs, offset = m_dataset->GetSlideOffset(slideName, slideObjs);
				float	*xList = m_dataset->GetXCentroidList(), *yList = m_dataset->GetYCentroidList();
				float	*xCent = &xList[offset], *yCent = &yList[offset], *slideScores = &m_scores[offset], uncertMedian;
				SlideStat   *stats = new SlideStat;

				stats->slide = slideName;
				stats->alphaIndex = index;
				stats->width = width;
				stats->height = height;
				m_statList.push_back(stats);

				workers.push_back(std::thread(&Learner::HeatmapWorker, this, slideScores,
										xCent, yCent, slideObjs, slideName, width, height,
										&m_statList[index]->uncertMin, &m_statList[index]->uncertMax,
										&m_statList[index]->classMin, &m_statList[index]->classMax,
										&m_statList[index]->uncertMedian));
			}

			for( auto &t : workers )
				t.join();
		}

		sort(m_statList.begin(), m_statList.end(), SortFunc);
		gLogger->LogMsg(EvtLogger::Evt_INFO, "GenAllHeatmaps took %f", gLogger->WallTime() - timing);

		// Indicate heatmaps have been updated
		//
		m_heatmapReload = false;

	} else {
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Training set not updated since heatmaps were last generated");
	}



	// TODO - Move the following to its own function
	//
	json_t	*slideList = NULL, *item = NULL;
	if( result ) {
		slideList = json_array();
		if( slideList == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to create JSON Array object");
			result = false;
		}
	}

	vector<SlideStat*>::iterator	it;

	if( result ) {

		for(it = m_statList.begin(); it != m_statList.end(); it++) {
			item = json_object();
			if( item == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to create JSON object");
				result = false;
				break;
			}

			json_object_set(item, "slide", json_string((*it)->slide.c_str()));
			json_object_set(item, "width", json_integer((*it)->width));
			json_object_set(item, "height", json_integer((*it)->height));
			json_object_set(item, "uncertMin", json_real((*it)->uncertMin));
			json_object_set(item, "uncertMax", json_real((*it)->uncertMax));
			json_object_set(item, "uncertMedian", json_real((*it)->uncertMedian));
			json_object_set(item, "classMin", json_real((*it)->classMin));
			json_object_set(item, "classMax", json_real((*it)->classMax));
			json_object_set(item, "index", json_integer((*it)->alphaIndex));

			json_array_append(slideList, item);
			json_decref(item);
		}


		char *jsonObj = json_dumps(slideList, 0);
		size_t  bytesWritten = :: write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(slideList);
		free(jsonObj);
	}

	return result;
}







void Learner::HeatmapWorker(float *slideScores, float *centX, float *centY, int numObjs,
							string slide, int width, int height, double *uncertMin, double *uncertMax,
							double *classMin, double *classMax, float *uncertMedian)
{
	bool	result = true;
	int		fX = (ceil((float)width / (float)GRID_SIZE)) + 1, fY = (ceil((float)height / (float)GRID_SIZE)) + 1,
			curX, curY;
	Mat		uncertainMap = Mat::zeros(fY, fX, CV_32F), classMap = Mat::zeros(fY, fX, CV_32F),
			densityMap = Mat::zeros(fY, fX, CV_32F), grayUncertain(fY, fX, CV_8UC1),
			grayClass(fY, fX, CV_8UC1);
	vector<float> scoreVec;



	for(int obj = 0; obj < numObjs; obj++) {
		curX = ceil(centX[obj] / (float)GRID_SIZE);
		curY = ceil(centY[obj] / (float)GRID_SIZE);

		uncertainMap.at<float>(curY, curX) = max(uncertainMap.at<float>(curY, curX), 1 - abs(slideScores[obj]));
		if( slideScores[obj] >= 0 ) {
			classMap.at<float>(curY, curX) += 1.0f;
		}
		densityMap.at<float>(curY, curX) += 1.0f;
		scoreVec.push_back(1 - abs(slideScores[obj]));
	}

	for(int row = 0; row < fY; row++) {
		for(int col = 0; col < fX; col++) {

			if( densityMap.at<float>(row, col) == 0 ) {
				classMap.at<float>(row, col) = 0;
			} else {
				classMap.at<float>(row, col) = classMap.at<float>(row, col) / densityMap.at<float>(row, col);
			}
		}
	}

	Size2i kernel(KERN_SIZE, KERN_SIZE);
	GaussianBlur(uncertainMap, uncertainMap, kernel, 3.5f);
	GaussianBlur(classMap, classMap, kernel, 3.5f);

	Mat		img, classImg;
	int		uncertHist[HIST_BINS], index, total;
	float	uncertNorm, range;

	// The min and max scores returned are the "blurred" veraion. Only
	// the median is the raw score. (calculated later)
	minMaxLoc(uncertainMap, uncertMin, uncertMax);
	minMaxLoc(classMap, classMin, classMax);


	range = *uncertMax - *uncertMin;

	memset(uncertHist, 0, HIST_BINS * sizeof(int));

	for(int row = 0; row < fY; row++) {
		for(int col = 0; col < fX; col++) {

			index = (int)min(uncertainMap.at<float>(row, col) / range * (float)HIST_BINS, (float)(HIST_BINS - 1));
			uncertHist[index]++;
		}
	}

	total = 0;
	for(index = 0; index < HIST_BINS; index++) {
		total += uncertHist[index];
		if( total > (int)(UNCERT_PERCENTILE * (float)fY * (float)fX) )
			break;
	}
	uncertNorm = (float)index / (float)HIST_BINS;

	for(int row = 0; row < fY; row++) {
		for(int col = 0; col < fX; col++) {

			grayUncertain.at<uchar>(row, col) = min(255.0 * uncertainMap.at<float>(row, col) / uncertNorm, 255.0);
			grayClass.at<uchar>(row, col) = min(255.0 * classMap.at<float>(row, col) / *classMax, 255.0);
		}
	}

	string	fqn = m_heatmapPath + "/" + slide + ".jpg",
			classFqn = m_heatmapPath + "/" + slide + "_class.jpg";

	applyColorMap(grayUncertain, img, COLORMAP_JET);
	vector<int> params;
	params.push_back(CV_IMWRITE_JPEG_QUALITY);
	params.push_back(75);
	result = imwrite(fqn.c_str(), img, params);

	applyColorMap(grayClass, classImg, COLORMAP_JET);
	result = imwrite(classFqn.c_str(), classImg, params);

	sort(scoreVec.begin(), scoreVec.end());

	// Return the median raw score, min and max should be blurred
	*uncertMedian = scoreVec[scoreVec.size() / 2];
}


bool Learner::GenHeatmapSRegion(const int sock, json_t *obj)
{
	bool	result = true;
	json_t	*value = NULL, *root = NULL;

	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);
	int		width, height;
	double	uncertMin, uncertMax, classMin, classMax, timing = gLogger->WallTime();
	float	uncertMedian;
	string	slide, uncertFileName, classFileName;

	result = IsUIDValid(uid);

	if( result ) {
		value = json_object_get(obj, "width");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatmapSRegion) Unable to decode width");
			result = false;
		} else {
			width = json_integer_value(value);
		}
	}

	if( result ) {
		value = json_object_get(obj, "height");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatmapSRegion) Unable to decode height");
			result = false;
		} else {
			height = json_integer_value(value);
		}
	}

	if( result ) {
		value = json_object_get(obj, "slide");
		if( value == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatmapSRegion) Unable to decode slide name");
			result = false;
		} else {
			slide = json_string_value(value);
		}
	}

	if( result ) {

		float	*scores = NULL, *xList = m_dataset->GetXCentroidList(), *yList = m_dataset->GetYCentroidList(),
				*centX, *centY;
		int		numObjs, offset;

		offset = m_dataset->GetSlideOffset(slide, numObjs);
		centX = &xList[offset];
		centY = &yList[offset];
		scores = (float*)malloc(numObjs * sizeof(float));

		gLogger->LogMsg(EvtLogger::Evt_INFO, "Viewer region heatmap generation took %f", gLogger->WallTime() - timing);

		gLogger->LogMsg(EvtLogger::Evt_INFO, "Slide name %s, numObjs %d, offset %d", slide.c_str(), numObjs, offset);

		if( scores == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatmapSRegion) Unable to allocate score buffer");
			result = false;
		}

		if( result ) {

			float 	**ptr = m_dataset->GetSlideData(slide, numObjs);
			int		dims = m_dataset->GetDims();

			result = m_classifier->ScoreBatch(ptr, numObjs, dims, scores);
			if( result == false ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatmapSRegion) Classification failed");
			}
		}

		if( result ) {
			uncertFileName = slide + ".jpg";
			classFileName = slide + "_class.jpg";


			string	fqfn = m_heatmapPath + "/" + uncertFileName;
			struct stat buffer;
			int		statResp = stat(fqfn.c_str(), &buffer);

			gLogger->LogMsg(EvtLogger::Evt_INFO, "GenHeatMapSRegion Start");
			// Check if heatmap needs to be generated
			if( m_heatmapReload || statResp != 0 ) {

				HeatmapWorkerSRegion(scores, centX, centY, numObjs, slide, width, height,
							  &uncertMin, &uncertMax, &classMin, &classMax, &uncertMedian);
			} else {

				vector<SlideStat*>::iterator	it;

				for(it = m_statList.begin(); it != m_statList.end(); it++) {
					if( (*it)->slide.compare(slide) == 0 ) {
						width = (*it)->width;
						height = (*it)->height;
						uncertMin = (*it)->uncertMin;
						uncertMax = (*it)->uncertMax;
						uncertMedian = (*it)->uncertMedian;
						classMin = (*it)->classMin;
						classMax = (*it)->classMax;
						break;
					}
				}
			}
		}
	}


	if( result ) {
		root = json_object();
		if( root == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenHeatmap) Unable to create JSON object");
			result = false;
		}
	}

	if( result ) {
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Viewer heatmapRegion generation took %f", gLogger->WallTime() - timing);

		json_object_set(root, "width", json_integer(width));
		json_object_set(root, "height", json_integer(height));
		json_object_set(root, "uncertFilename", json_string(uncertFileName.c_str()));
		json_object_set(root, "classFilename", json_string(classFileName.c_str()));
		json_object_set(root, "uncertMin", json_real(uncertMin));
		json_object_set(root, "uncertMax", json_real(uncertMax));
		json_object_set(root, "uncertMedian", json_real(uncertMedian));
		json_object_set(root, "classMin", json_real(classMin));
		json_object_set(root, "classMax", json_real(classMax));

		char *jsonObj = json_dumps(root, 0);
		size_t  bytesWritten = :: write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(root);
		free(jsonObj);
	}
	return result;
}


bool Learner::GenAllHeatmapsSRegion(const int sock, json_t *obj)
{
	bool result = true;
	json_t	*value = NULL, *slideData = NULL;

	value = json_object_get(obj, "uid");
	const char *uid = json_string_value(value);
	result = IsUIDValid(uid);

	if( result && m_heatmapReload ) {
		double	start, timing = gLogger->WallTime();

		if( result ) {
			slideData = json_object_get(obj, "slides");
			if( slideData == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode slide data");
				result = false;
			}
		}

		json_t	*slides = NULL, *x_size = NULL, *y_size = NULL;

		if( result ) {

			slides = json_object_get(slideData, "slides");
			if( !json_is_array(slides) ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode slide name array");
				result = false;
			}
		}

		if( result ) {

			x_size = json_object_get(slideData, "x_size");
			if( !json_is_array(x_size) ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode x_size array");
				result = false;
			}
		}

		if( result ) {

			y_size = json_object_get(slideData, "y_size");
			if( !json_is_array(y_size) ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode y_size array");
				result = false;
			}
		}

		// To identify the most recent scores,
		// Classify all objects for heatmap generation
		//
		float 	**ptr = m_dataset->GetData();
		int	dims = m_dataset->GetDims();
		result = m_classifier->ScoreBatch(ptr, m_dataset->GetNumObjs(), dims, m_scores);
		if( result == false ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Classification failed");
		}

		if( result ) {
			size_t index, numSlides = json_array_size(slides);
			const char 	*slideName = NULL;
			int			width, height;
			vector<std::thread> workers;


			if( !m_statList.empty() ) {
				for(int i = 0; i < m_statList.size(); i++) {
					delete m_statList[i];
				}
				m_statList.clear();
			}

			json_array_foreach(slides, index, value) {

				slideName = json_string_value(value);
				if( slideName == NULL ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode slide name");
					result = false;
					break;
				}

				value = json_array_get(x_size, index);
				if( value == NULL ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode width");
					result = false;
					break;
				}
				width = json_integer_value(value);

				value = json_array_get(y_size, index);
				if( value == NULL ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to decode height");
					result = false;
					break;
				}
				height = json_integer_value(value);

				int slideObjs, offset = m_dataset->GetSlideOffset(slideName, slideObjs);
				float	*xList = m_dataset->GetXCentroidList(), *yList = m_dataset->GetYCentroidList();
				float	*xCent = &xList[offset], *yCent = &yList[offset], *slideScores = &m_scores[offset], uncertMedian;
				SlideStat   *stats = new SlideStat;

				stats->slide = slideName;
				stats->alphaIndex = index;
				stats->width = width;
				stats->height = height;
				m_statList.push_back(stats);

				workers.push_back(std::thread(&Learner::HeatmapWorkerSRegion, this, slideScores,
										xCent, yCent, slideObjs, slideName, width, height,
										&m_statList[index]->uncertMin, &m_statList[index]->uncertMax,
										&m_statList[index]->classMin, &m_statList[index]->classMax,
										&m_statList[index]->uncertMedian));
			}

			for( auto &t : workers )
				t.join();
		}

		sort(m_statList.begin(), m_statList.end(), SortFunc);
		gLogger->LogMsg(EvtLogger::Evt_INFO, "GenAllHeatmaps_SRegion took %f", gLogger->WallTime() - timing);

		// Indicate heatmaps have been updated
		//
		m_heatmapReload = false;

	} else {
		gLogger->LogMsg(EvtLogger::Evt_INFO, "Training set not updated since heatmaps were last generated");
	}



	// TODO - Move the following to its own function
	//
	json_t	*slideList = NULL, *item = NULL;
	if( result ) {
		slideList = json_array();
		if( slideList == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to create JSON Array object");
			result = false;
		}
	}

	vector<SlideStat*>::iterator	it;

	if( result ) {

		for(it = m_statList.begin(); it != m_statList.end(); it++) {
			item = json_object();
			if( item == NULL ) {
				gLogger->LogMsg(EvtLogger::Evt_ERROR, "(Learner::GenAllheatmaps) Unable to create JSON object");
				result = false;
				break;
			}

			json_object_set(item, "slide", json_string((*it)->slide.c_str()));
			json_object_set(item, "width", json_integer((*it)->width));
			json_object_set(item, "height", json_integer((*it)->height));
			json_object_set(item, "uncertMin", json_real((*it)->uncertMin));
			json_object_set(item, "uncertMax", json_real((*it)->uncertMax));
			json_object_set(item, "uncertMedian", json_real((*it)->uncertMedian));
			json_object_set(item, "classMin", json_real((*it)->classMin));
			json_object_set(item, "classMax", json_real((*it)->classMax));
			json_object_set(item, "index", json_integer((*it)->alphaIndex));

			json_array_append(slideList, item);
			json_decref(item);
		}


		char *jsonObj = json_dumps(slideList, 0);
		size_t  bytesWritten = :: write(sock, jsonObj, strlen(jsonObj));

		if( bytesWritten != strlen(jsonObj) )
			result = false;

		json_decref(slideList);
		free(jsonObj);
	}

	return result;
}


void Learner::HeatmapWorkerSRegion(float *slideScores, float *centX, float *centY, int numObjs,
							string slide, int width, int height, double *uncertMin, double *uncertMax,
							double *classMin, double *classMax, float *uncertMedian)
{
		bool	result = true;
		int		fX = (ceil((float)width / (float)SREGION_GRID_SIZE)) + 1, fY = (ceil((float)height / (float)SREGION_GRID_SIZE)) + 1,
				curX, curY;
		Mat		uncertainMap = Mat::zeros(fY, fX, CV_32F), classMap = Mat::zeros(fY, fX, CV_32F),
				densityMap = Mat::zeros(fY, fX, CV_32F), grayUncertain(fY, fX, CV_8UC1),
				grayClass(fY, fX, CV_8UC1);
		vector<float> scoreVec;

		for(int obj = 0; obj < numObjs; obj++) {
			scoreVec.push_back(1 - abs(slideScores[obj]));
		}

		sort(scoreVec.begin(), scoreVec.end());
		// Return the median raw score, min and max should be blurred
		*uncertMedian = scoreVec[scoreVec.size() / 2];

		for(int obj = 0; obj < numObjs; obj++) {
			curX = ceil(centX[obj] / (float)SREGION_GRID_SIZE);
			curY = ceil(centY[obj] / (float)SREGION_GRID_SIZE);
			//uncertainMap.at<float>(curY, curX) = max(uncertainMap.at<float>(curY, curX), 1 - abs(slideScores[obj]));
			float uncertainty = (1 - abs(slideScores[obj]));

			if (uncertainty > *uncertMedian){
				uncertainMap.at<float>(curY, curX) += 1.0f;
			}

			if( slideScores[obj] >= 0 ) {
				classMap.at<float>(curY, curX) += 1.0f;
			}
			densityMap.at<float>(curY, curX) += 1.0f;
		}


		for(int row = 0; row < fY; row++) {
			for(int col = 0; col < fX; col++) {

				if( densityMap.at<float>(row, col) == 0 ) {
					classMap.at<float>(row, col) = 0;
					uncertainMap.at<float>(row, col) = 0;
				} else {
					classMap.at<float>(row, col) = classMap.at<float>(row, col) / densityMap.at<float>(row, col);
					uncertainMap.at<float>(row, col) = uncertainMap.at<float>(row, col) / densityMap.at<float>(row, col);
				}
			}
		}

		Size2i kernel(KERN_SIZE, KERN_SIZE);
		GaussianBlur(uncertainMap, uncertainMap, kernel, 3.5f);
		GaussianBlur(classMap, classMap, kernel, 3.5f);

		Mat		img, classImg;

		// The min and max scores returned are the "blurred" veraion. Only
		// the median is the raw score. (calculated later)
		minMaxLoc(uncertainMap, uncertMin, uncertMax);
		minMaxLoc(classMap, classMin, classMax);

		for(int row = 0; row < fY; row++) {
			for(int col = 0; col < fX; col++) {

				grayUncertain.at<uchar>(row, col) = min(255.0 * uncertainMap.at<float>(row, col)/ *uncertMax, 255.0);
				grayClass.at<uchar>(row, col) = min(255.0 * classMap.at<float>(row, col) / *classMax, 255.0);
			}
		}

		string	fqn = m_heatmapPath + "/" + slide + ".jpg",
				classFqn = m_heatmapPath + "/" + slide + "_class.jpg";

		applyColorMap(grayUncertain, img, COLORMAP_JET);
		vector<int> params;
		params.push_back(CV_IMWRITE_JPEG_QUALITY);
		params.push_back(75);
		result = imwrite(fqn.c_str(), img, params);

		applyColorMap(grayClass, classImg, COLORMAP_JET);
		result = imwrite(classFqn.c_str(), classImg, params);

}
