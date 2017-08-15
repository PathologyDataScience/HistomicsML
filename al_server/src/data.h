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
#if !defined(_DATA_H_)
#define _DATA_H_

#include <string>
#include <vector>

#include "hdf5.h"
#include "hdf5_hl.h"




using namespace std;


class MData
{
public:
		MData(void);
		~MData(void);


		bool	Load(string fileName);
		bool	Create(float *dataSet, int numObjs, int numDims, int *labels,
						int *ids, int *iteration, float *means, float *stdDev,
						float *xCentroid, float *yCentroid, char **slides,
						int *slideIdx, int slideCnt, vector<string>& classNames,
						float *xClick = NULL, float *yClick = NULL);

		bool	SaveAs(string filename);

		float	**GetData(void) { return m_objects; }
		int		*GetLabels(void) { return m_haveLabels ? m_labels : NULL;  }
		char	**GetSlideNames(void)  { return m_slides; }
		int		*GetSlideIndices(void) { return m_slideIdx; }

		int		GetNumObjs(void) { return m_numObjs; }
		int		GetDims(void) { return m_numDim; }
		int		GetNumSlides(void) { return m_numSlides; }
		bool	HaveLabels(void) { return m_haveLabels; }

		int		GetNumClasses(void)		{	return m_numClasses; }
		char	**GetClassNames(void) { return m_classNames; }

		int		FindItem(float xCent, float yCent, string slide);
		bool	GetSample(int index, float* sample);
		char	*GetSlide(int index);
		int		GetSlideIdx(const char *slide);
		float	GetXCentroid(int index);
		float	GetYCentroid(int index);
		float	*GetXCentroidList(void) { return m_xCentroid; }
		float	*GetYCentroidList(void) { return m_yCentroid; }
		float	*GetXClickList(void) { return m_xClick; }
		float	*GetYClickList(void) { return m_yClick; }
		int		GetIteration(int index) { return m_iteration ? m_iteration[index] : 0;  }
		int		*GetIterationList(void) { return m_iteration; }
		float	*GetMeans(void) { return m_means; }
		float	*GetStdDevs(void)  { return m_stdDevs; }
		int		*GetIdList(void)  { return m_dbIds;  }

		float 	**GetSlideData(const string slide, int& numSlideObjs);
		int		GetSlideOffset(const string slide, int& numSlideObjs);

protected:

		float	**m_objects;
		int		*m_labels;
		float	*m_xCentroid;
		float	*m_yCentroid;
		int		*m_slideIdx;
		char	**m_slides;
		char	**m_classNames;
		float	*m_xClick;
		float	*m_yClick;

		bool	m_haveIters;
		int		*m_iteration;

		bool	m_haveLabels;
		int		m_numClasses;
		int		m_numObjs;
		int		m_numDim;
		int		m_numSlides;
		int		m_stride;			// # of objects per memory chunk

		bool	m_haveDbIds;
		int		*m_dbIds;

		bool 	m_created;

		int		*m_dataIdx;		// Index where the corresponding slide's feature
								// data starts.

		// Normalization parameters
		//
		float	*m_means;
		float	*m_stdDevs;

		hid_t	m_slideSpace;			// For cleaning up slide and class names
		hid_t	m_slideMemType;
		hid_t	m_classNameSpace;
		hid_t	m_classNameMemType;
	
		bool	ReadFeatureData(hid_t fileId);
		void 	Cleanup(void);
		bool	SaveProvenance(hid_t fileId);
		bool	CreateSlideData(char **slides, int *slideIdx, int numSlides, int numObjs);
		bool	CreateClassNames(vector<string>& classNames);
};


#endif
