//
//	Copyright (c) 2014-2018, Emory University
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
#include <iostream>
#include <fstream>
#include <set>
#include <dirent.h>
#include <string.h>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <boost/algorithm/string.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/contrib/contrib.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "ocvsvm.h"
#include "ocvrandforest.h"
#include "data.h"
#include "logger.h"

#include "val-cmdline.h"
#include "base_config.h"

#include "hdf5.h"
#include "hdf5_hl.h"
#include "mysql_connection.h"
#include "tiffio.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

using namespace std;
using namespace cv;

EvtLogger	*gLogger = NULL;

// Default tile size
#define TILE_SIZE 4096
#define SUPERPIXEL_SIZE 64

struct Cent {
	float		x;
	float		y;
};


int GenerateMaskRegions(MData& trainSet, MData& testSet, Classifier *classifier,
					string slidesInfo)
{
	int		result = 0, dims = trainSet.GetDims(), *trainLabel = trainSet.GetLabels(),
			numTestObjs = testSet.GetNumObjs(), numSlides = testSet.GetNumSlides();

	float	**test = testSet.GetData(), **train = trainSet.GetData(),
			*predScore = NULL;

	if( dims != testSet.GetDims() ) {
		cerr << "Training and test set dimensions do not match" << endl;
		result = -30;
	}

	if( result == 0 ) {
		cout << "Allocating prediction buffer" << endl;
		predScore = (float*)malloc(numTestObjs * sizeof(float));
		if( predScore == NULL ) {
			cerr << "Unable to allocae prediction buffer" << endl;
			result = -31;
		}
	}

	if( result == 0 ) {
		cout << "Training classifier..." << endl;
		if( !classifier->Train(train[0], trainLabel, trainSet.GetNumObjs(), dims) ) {
			cerr << "Classifier training failed" << endl;
			result = -32;
		}
	}

	if( result == 0 ) {
		cout << "Applying classifier..." << endl;
		if( ! classifier->ScoreBatch(test, numTestObjs, dims, predScore) ) {
			cerr << "Applying classifier failed" << endl;
			result = -33;
		}
	}

	cout << "Loading test dataset ..." << endl;

	float	*m_xCentroid = testSet.GetXCentroidList();
	float	*m_yCentroid = testSet.GetYCentroidList();

	cout << "Loading annotation info ..." << endl;

	ifstream file(slidesInfo);

	vector<vector<string> > dataList;
	string line = "";

	while(getline(file, line)) {
			vector<string> vec;
			boost::algorithm::split(vec, line, boost::is_any_of(","));
			dataList.push_back(vec);
	}

	file.close();

	// get slide name from dataset
	char	**slideNames = testSet.GetSlideNames();

	for(vector<string> vec : dataList) {

			if (vec.size() != 5) {
					cerr << "Number of column must be 5 ..." << endl;
					result = -1;
			}

			if( result == 0 ) {

				string slideName = vec.at(0);
				int start_x = stoi(vec.at(1));
				int start_y = stoi(vec.at(2));
				int width = stoi(vec.at(3));
				int height = stoi(vec.at(4));
				bool isSlide = false;
				// check if the slide exists
				for(int i = 0; i < testSet.GetNumSlides() - 1; i++) {
					if (strcmp(slideNames[i], slideName.c_str()) == 0) {
							isSlide = true;
							break;
					}
				}

				if(isSlide) {

						cout << "Generating masks for " << slideName << endl;

						int slide_width;
						int slide_height;
						int slide_scale;
						string outFileName = slideName + "_" + to_string(start_x)
																	+ "_" + to_string(start_y)
																	+ "_" + to_string(width)
																	+ "_" + to_string(height) + "_.tif";

						if( result == 0 ) {
								int slideObjs, offset = testSet.GetSlideOffset(slideName, slideObjs);
								float	*m_predScore = &predScore[offset];
								// cout << "Running mysql ..." << endl;

								try {
									sql::Driver *driver;
									sql::Connection *con;
									sql::Statement *stmt;
									sql::ResultSet *res;

									// Create a connection
									driver = get_driver_instance();
									con = driver->connect("tcp://127.0.0.1:3306", "guest", "valsGuets");
									// Connect to the MySQL test database
									con->setSchema("nuclei");
									stmt = con->createStatement();
									// Query to get slide width, height, scale
									res = stmt->executeQuery(string("SELECT x_size, y_size, scale FROM slides where name= '"+ slideName + '\'' + " limit 1").c_str());
									while (res->next()) {
											 slide_width = res->getInt(1);
											 slide_height = res->getInt(2);
											 slide_scale = res->getInt(3);
									}

									// Set 20x tile size and superpixel size as a default
									int tile_size;
									int p_size;

									// If 40x, then multiply by 2
									if (slide_scale == 2) {
										tile_size = TILE_SIZE*2;
										p_size = SUPERPIXEL_SIZE*2;
									}

									// In order to get correct ROI from centroids and boundaries,
									// we need to make ROI to be bold.
									// We will get the initaial ROI back at the end.

									int tile_num_y;
									int tile_num_x;

									// Get total tile numbers
									tile_num_x = slide_width/tile_size + tile_size;
									tile_num_y = slide_height/tile_size + tile_size;

									int bold_start_x = start_x;

									if (start_x > p_size)
										bold_start_x = start_x - p_size;

									int bold_start_y = start_y;

									if (start_y > p_size)
										bold_start_y = start_y - p_size;

									int bold_end_x = start_x + width + p_size;
									int bold_end_y = start_y + height + p_size;

									int bold_width = bold_end_x - bold_start_x;
									int bold_height =  bold_end_y - bold_start_y;

									Mat mask(height, width, CV_32F, Scalar(7.0));
									Mat bold_mask(bold_height, bold_width, CV_32F, Scalar(7.0));

									for(int i = offset; i < offset+slideObjs; i++) {
											if ((m_xCentroid[i] >= bold_start_x) && (m_xCentroid[i] <= bold_end_x) && (m_yCentroid[i] >= bold_start_y) && (m_yCentroid[i] <= bold_end_y)) {

												stringstream stream_x;
												stream_x << fixed << setprecision(1) << m_xCentroid[i];
												string s_x = stream_x.str();

												stringstream stream_y;
												stream_y << fixed << setprecision(1) << m_yCentroid[i];
												string s_y = stream_y.str();
												res = stmt->executeQuery(string("SELECT boundary FROM sregionboundaries where slide= '"+ slideName + '\''+ \
												" and centroid_x="+ s_x +" and centroid_y="+ s_y +" limit 1").c_str());

												while (res->next()) {

														string t = res->getString(1);
														vector<string> strs;
														boost::split(strs, t ,boost::is_any_of(" "));
														vector<Point> pts;

														for (size_t i = 0; i < strs.size() - 1; i++) {
																vector<string> coords;
																boost::split(coords, strs[i], boost::is_any_of(","));
																int x = stoi(coords[0])-bold_start_x;
																int y = stoi(coords[1])-bold_start_y;
																pts.push_back(Point(x, y));
														}
														vector<vector<Point>> ptsAll;
														ptsAll.push_back(pts);
														fillPoly(bold_mask, ptsAll, m_predScore[i - offset]);
												}
											}
									}

									int left = start_x - bold_start_x;
									int top = start_y - bold_start_y;

									cout << "Writing " << outFileName << " ..." << endl;
									// copy to the original region
									bold_mask(Rect(left, top, width, height)).copyTo(mask);

									TIFF* out = TIFFOpen(outFileName.c_str(), "w");

									// float* image = new float[width * height];
									short int compression = COMPRESSION_LZW;

									TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
									TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
									TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
									TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 32);
									TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
									TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
									TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
									TIFFSetField(out, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
									//TIFFSetField(out, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
									TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
									TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);

									// length in memory of one row of pixel in the image.
									tsize_t linebytes = width;

									// set the strip size of the file to be size of one row of pixels
									TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, width*4));

									// writing image to the file one strip at a time
									for (uint32 row = 0; row < height; row++) {
											if (TIFFWriteScanline(out, mask.ptr<float>(row), row, 0) < 0)
											break;
									}

									TIFFClose(out);

									delete res;
									delete stmt;
									delete con;

								} catch (sql::SQLException &e) {
									cout << " ERR: SQLException in ";
									cout << " Error code: " << e.getErrorCode();
									cout << " State: " << e.getSQLState() << endl;
								}

						} // result = 0 end

				}

			}

	}

	if( predScore ) {
			free(predScore);
			predScore = NULL;
	}

	return result;
}


int GenerateMaskRegion(MData& trainSet, MData& testSet, Classifier *classifier,
					string testFile, string slideName, int start_x, int start_y, int width, int height,
					string outFileName)
{
	int		result = 0, dims = trainSet.GetDims(), *trainLabel = trainSet.GetLabels(),
			numTestObjs = testSet.GetNumObjs(), numSlides = testSet.GetNumSlides();

	float	**test = testSet.GetData(), **train = trainSet.GetData(),
			*predScore = NULL;

	if( dims != testSet.GetDims() ) {
		cerr << "Training and test set dimensions do not match" << endl;
		result = -30;
	}

	if( result == 0 ) {
		cout << "Allocating prediction buffer" << endl;
		predScore = (float*)malloc(numTestObjs * sizeof(float));
		if( predScore == NULL ) {
			cerr << "Unable to allocae prediction buffer" << endl;
			result = -31;
		}
	}

	if( result == 0 ) {
		cout << "Training classifier..." << endl;
		if( !classifier->Train(train[0], trainLabel, trainSet.GetNumObjs(), dims) ) {
			cerr << "Classifier training failed" << endl;
			result = -32;
		}
	}

	if( result == 0 ) {
		cout << "Applying classifier..." << endl;
		if( ! classifier->ScoreBatch(test, numTestObjs, dims, predScore) ) {
			cerr << "Applying classifier failed" << endl;
			result = -33;
		}
	}

	cout << "Loading test dataset ..." << endl;

	float	*m_xCentroid = testSet.GetXCentroidList();
	float	*m_yCentroid = testSet.GetYCentroidList();

	cout << "Generating mask..." << endl;

	// get slide name
	char	**slideNames = testSet.GetSlideNames();

	// check if the slide exists
	bool isSlide = false;

	for(int i = 0; i < testSet.GetNumSlides() - 1; i++) {
		if (strcmp(slideNames[i], slideName.c_str()) == 0) {
				isSlide = true;
				break;
		}
	}

	int slide_width;
	int slide_height;
	int slide_scale;

	if( result == 0 ) {
			int slideObjs, offset = testSet.GetSlideOffset(slideName, slideObjs);
			float	*m_predScore = &predScore[offset];
			//cout << "Running mysql ..." << endl;

			try {
				sql::Driver *driver;
				sql::Connection *con;
				sql::Statement *stmt;
				sql::ResultSet *res;

				// Create a connection
				driver = get_driver_instance();
				con = driver->connect("tcp://127.0.0.1:3306", "guest", "valsGuets");
				// Connect to the MySQL test database
				con->setSchema("nuclei");
				stmt = con->createStatement();
				// Query to get slide width, height, scale
				res = stmt->executeQuery(string("SELECT x_size, y_size, scale FROM slides where name= '"+ slideName + '\'' + " limit 1").c_str());
				while (res->next()) {
						 slide_width = res->getInt(1);
						 slide_height = res->getInt(2);
						 slide_scale = res->getInt(3);
				}

				// Set 20x tile size and superpixel size as a default
				int tile_size;
				int p_size;
				// If 40x, then multiply by 2
				if (slide_scale == 2) {
					tile_size = TILE_SIZE*2;
					p_size = SUPERPIXEL_SIZE*2;
				}

				// In order to get correct ROI from centroids and boundaries,
				// we need to make ROI to be bold.
				// We will get the initaial ROI back at the end.

				int tile_num_y;
				int tile_num_x;

				// Get total tile numbers
				tile_num_x = slide_width/tile_size + tile_size;
				tile_num_y = slide_height/tile_size + tile_size;

				int bold_start_x = start_x;

				if (start_x > p_size)
					bold_start_x = start_x - p_size;

				int bold_start_y = start_y;

				if (start_y > p_size)
					bold_start_y = start_y - p_size;

				int bold_end_x = start_x + width + p_size;
				int bold_end_y = start_y + height + p_size;

				int bold_width = bold_end_x - bold_start_x;
				int bold_height =  bold_end_y - bold_start_y;

				Mat mask(height, width, CV_32F, Scalar(7.0));
				Mat bold_mask(bold_height, bold_width, CV_32F, Scalar(7.0));

				for(int i = offset; i < offset+slideObjs; i++) {
						if ((m_xCentroid[i] >= bold_start_x) && (m_xCentroid[i] <= bold_end_x) && (m_yCentroid[i] >= bold_start_y) && (m_yCentroid[i] <= bold_end_y)) {

							stringstream stream_x;
							stream_x << fixed << setprecision(1) << m_xCentroid[i];
							string s_x = stream_x.str();

							stringstream stream_y;
							stream_y << fixed << setprecision(1) << m_yCentroid[i];
							string s_y = stream_y.str();

							res = stmt->executeQuery(string("SELECT boundary FROM sregionboundaries where slide= '"+ slideName + '\''+ \
							" and centroid_x="+ s_x +" and centroid_y="+ s_y +" limit 1").c_str());

							while (res->next()) {

									string t = res->getString(1);
									vector<string> strs;
									boost::split(strs,t ,boost::is_any_of(" "));
									vector<Point> pts;

									for (size_t i = 0; i < strs.size() - 1; i++) {
											vector<string> coords;
											boost::split(coords,strs[i] ,boost::is_any_of(","));
											int x = stoi(coords[0])-bold_start_x;
											int y = stoi(coords[1])-bold_start_y;
											pts.push_back(Point(x, y));
									}

									vector<vector<Point>> ptsAll;
									ptsAll.push_back(pts);
									fillPoly(bold_mask, ptsAll, m_predScore[i - offset]);
							}
						}
				}

				int left = start_x - bold_start_x;
				int top = start_y - bold_start_y;

				cout << "Writing " << outFileName << " ..." << endl;
				// copy to the original region
				bold_mask(Rect(left, top, width, height)).copyTo(mask);

				TIFF* out = TIFFOpen(outFileName.c_str(), "w");

				// float* image = new float[width * height];
				short int compression = COMPRESSION_LZW;

				TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
				TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
				TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
				TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 32);
				TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
				TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
				TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
				TIFFSetField(out, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
				//TIFFSetField(out, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
				TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
				TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);

				// length in memory of one row of pixel in the image.
				tsize_t linebytes = width;

				// set the strip size of the file to be size of one row of pixels
				TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, width*4));

				// writing image to the file one strip at a time
				for (uint32 row = 0; row < height; row++) {
						if (TIFFWriteScanline(out, mask.ptr<float>(row), row, 0) < 0)
						break;
				}

				TIFFClose(out);

				delete res;
				delete stmt;
				delete con;

			} catch (sql::SQLException &e) {
				cout << " ERR: SQLException in ";
				cout << " Error code: " << e.getErrorCode();
				cout << " State: " << e.getSQLState() << endl;
			}

	} // result = 0 end

	if( predScore )
		free(predScore);

	return result;
}



// Renormalize the training set using the test set's mean and std dev.
int Renormalize(MData& trainSet, MData& testSet)
{
	int		result = 0;
	float	*trainMean = trainSet.GetMeans(), *testMean = testSet.GetMeans(),
			*trainStdDev = trainSet.GetStdDevs(), *testStdDev = testSet.GetStdDevs(),
			**trainFeatures = trainSet.GetData();
	bool	norm = true;

	// Check if re-normalization is actually needed
	for(int i = 0; i < trainSet.GetDims(); i++) {
		if( trainMean[i] != testMean[i] ||
			trainStdDev[i] != testStdDev[i] ) {
				norm = false;
				break;
		}
	}


	if( norm ) {
		for(int obj = 0; obj < trainSet.GetNumObjs(); obj++) {
			for(int dim = 0; dim < trainSet.GetDims(); dim++) {
				trainFeatures[obj][dim] = (trainFeatures[obj][dim] * trainStdDev[dim]) + trainMean[dim];

				trainFeatures[obj][dim] = (trainFeatures[obj][dim] - testMean[dim]) / testStdDev[dim];
			}
		}
	}
	return result;
}







int TrainClassifier(Classifier *classifier, MData& trainSet)
{
	int		result = 0;
	int		*labels = trainSet.GetLabels(), numObjs = trainSet.GetNumObjs(),
			numDims = trainSet.GetDims();
	float	**data = trainSet.GetData();

	if( !classifier->Train(data[0], labels, numObjs, numDims) ) {
		cerr << "Classifier traiing FAILED" << endl;
		result = -20;
	}
	return result;
}






int CountResults(MData& testSet, int *predictions, int *&posSlideCnt, int *&negSlideCnt)
{
	int		result = 0;
	int		*slideIdx = testSet.GetSlideIndices();

	posSlideCnt = (int*)calloc(testSet.GetNumSlides(), sizeof(int));
	negSlideCnt = (int*)calloc(testSet.GetNumSlides(), sizeof(int));

	if( negSlideCnt && posSlideCnt ) {
		for(int i = 0; i < testSet.GetNumObjs(); i++) {
			if( predictions[i] == 1 ) {
				posSlideCnt[slideIdx[i]]++;
			} else {
				negSlideCnt[slideIdx[i]]++;
			}
		}
	}
	return result;
}





//  Apply the classifier to each slide in the test set. Save the counts of positive and negative
//	classes for each slide to a csv file.
//
int	ClassifySlides(MData& trainSet, MData& testSet, Classifier *classifier, string testFile,
					string outFileName)
{
	int		result = 0;
	int		*pred = NULL, *posSlideCnt = NULL, *negSlideCnt = NULL;
	char	**slideNames = testSet.GetSlideNames();
	ofstream 	outFile(outFileName.c_str());

	if( classifier == NULL ) {
		result = -10;
	}

	if( result == 0 ) {
		if( outFile.is_open() ) {

			outFile << "slides,";
			for(int i = 0; i < testSet.GetNumSlides() - 1; i++) {
				outFile << slideNames[i] << ",";
			}
			outFile << slideNames[testSet.GetNumSlides() - 1] << endl;

			pred = (int*)malloc(testSet.GetNumObjs() * sizeof(int));
			if( pred == NULL ) {
				cerr << "Unable to allocate results buffer" << endl;
				result = -11;
			}

		} else {
			cerr << "Unable to open " << outFileName << endl;
			result = -12;
		}
	}

	if( result == 0 ) {
		result = TrainClassifier(classifier, trainSet);
	}


	if( result == 0 ) {
		if( !classifier->ClassifyBatch(testSet.GetData(), testSet.GetNumObjs(), testSet.GetDims(), pred) ) {
			cerr << "Classification failed" << endl;
			result = -13;
		}
	}

	if( result == 0 ) {
		result = CountResults(testSet, pred, posSlideCnt, negSlideCnt);
	}

	if( result == 0 ) {
		outFile << "positive,";
		for( int i = 0; i < testSet.GetNumSlides() - 1; i++ ) {
			 outFile <<  posSlideCnt[i] << ",";
		}
		outFile <<  posSlideCnt[testSet.GetNumSlides() - 1] << endl;

		outFile << "negative,";
		for( int i = 0; i < testSet.GetNumSlides() - 1; i++ ) {
			outFile <<  negSlideCnt[i] << ",";
		}
		outFile <<  negSlideCnt[testSet.GetNumSlides() - 1] << endl;
	}

	if( outFile.is_open() ) {
		outFile.close();
	}

	if( posSlideCnt )
		free(posSlideCnt);
	if( negSlideCnt )
		free(negSlideCnt);
	return result;
}






int	CalcROC(MData& trainSet, MData& testSet, Classifier *classifier, string testFile,
			string outFileName)
{
	int		result = 0, *trainLabels = trainSet.GetLabels(),
			*testLabels = testSet.GetLabels(),
			*predClass = (int*)malloc(testSet.GetNumObjs() * sizeof(int));

	float	**train = trainSet.GetData(), **test = testSet.GetData(),
			*scores = (float*)malloc(testSet.GetNumObjs() * sizeof(float));

	if( testLabels == NULL ) {
		cerr << "Test set has no lables" << endl;
		result = -10;
	}

	if( predClass == NULL || scores == NULL ) {
		result = -11;
		cerr << "Unable to allocate results buffer" << endl;
	}

	ofstream 	outFile(outFileName.c_str());
	if( !outFile.is_open() ) {
		cerr << "Unable to create " << outFileName << endl;
		result = -12;
	}

	if( result == 0 ) {
		int TP = 0, FP = 0, TN = 0, FN = 0, P = 0, N = 0;

		cout << "Saving to: " << outFileName << endl;


		outFile << "labels,";
		for(int idx = 0; idx < testSet.GetNumObjs() - 1; idx++)
			outFile << testLabels[idx] << ",";
		outFile << testLabels[testSet.GetNumObjs() - 1] << endl;

		result = TrainClassifier(classifier, trainSet);
		classifier->ScoreBatch(test, testSet.GetNumObjs(), testSet.GetDims(), scores);

		for(int i = 0; i < testSet.GetNumObjs(); i++) {
			if( scores[i] >= 0.0f ) {
				predClass[i] = 1;
			} else {
				predClass[i] = -1;
			}
		}

		// Calculate confusion matrix
		TP = FP = TN = FN = P = N = 0;

		for(int i = 0; i < testSet.GetNumObjs(); i++) {
			if( testLabels[i] == 1 ) {
				P++;
				if( predClass[i] == 1 ) {
					TP++;
				} else {
					FN++;
				}
			} else {
				N++;
				if( predClass[i] == -1 ) {
					TN++;
				} else {
					FP++;
				}
			}
		}
		printf("\t%4d\t%4d\n", TP, FP);
		printf("\t%4d\t%4d\n\n", FN, TN);

		cout << "FP rate: " << (float)FP / (float)N << endl;
		cout << "TP rate: " << (float)TP / (float)P << endl;
		cout << "Precision: " << (float)TP / (float)(TP + FP) << endl;
		cout << "Accuracy: " << (float)(TP + TN) / (float)(N + P) << endl;

		outFile << "scores,";
		for(int idx = 0; idx < testSet.GetNumObjs() - 1; idx++) {
			outFile << ((scores[idx] + 1.0f) / 2.0) << ",";
		}
		outFile << ((scores[testSet.GetNumObjs() - 1] + 1.0f) / 2.0) << endl;

		outFile << "FP rate: ";
		outFile << (float)FP / (float)N << endl;
		outFile << "TP rate: ";
		outFile << (float)TP / (float)P << endl;
		outFile << "Precision: ";
		outFile << (float)TP / (float)(TP + FP) << endl;
		outFile << "Accuracy: ";
		outFile << (float)(TP + TN) / (float)(N + P) << endl;

		outFile.close();
	}

	if( predClass )
		free(predClass);
	if( scores )
		free(scores);

	return result;
}






int GenerateMap(MData& trainSet, MData& testSet, Classifier *classifier,
				string slide, string outFileName)
{
	int		result = 0, offset, slideObjs;
	float	**train = trainSet.GetData(), **test = testSet.GetData(),
			*scores = (float*)malloc(testSet.GetNumObjs() * sizeof(float));

	result = TrainClassifier(classifier, trainSet);
	classifier->ScoreBatch(test, testSet.GetNumObjs(), testSet.GetDims(), scores);

	offset = testSet.GetSlideOffset(slide, slideObjs);

	ofstream 	outFile(outFileName.c_str());

	if( outFile.is_open() ) {

		outFile << "score,X,Y" << endl;

		for(int i = offset; i < offset + slideObjs; i++) {
			outFile << scores[i] << "," << testSet.GetXCentroid(i) << "," << testSet.GetYCentroid(i) << endl;
		}
		outFile.close();
	} else {
		cerr << "Unable to create " << outFileName << endl;
		result = -10;
	}
	if( scores )
		free(scores);

	return result;
}





int ApplyClassifier(MData& trainSet, MData& testSet, Classifier *classifier,
					string testFileName, string outFileName)
{
	int		result = 0, dims = trainSet.GetDims(), *trainLabel = trainSet.GetLabels(),
			numTestObjs = testSet.GetNumObjs();
	float	**test = testSet.GetData(), **train = trainSet.GetData(),
			*predScore = NULL;

	if( dims != testSet.GetDims() ) {
		cerr << "Training and test set dimensions do not match" << endl;
		result = -30;
	}

	if( result == 0 ) {
		cout << "Allocating prediction buffer" << endl;
		predScore = (float*)malloc(numTestObjs * sizeof(float));
		if( predScore == NULL ) {
			cerr << "Unable to allocae prediction buffer" << endl;
			result = -31;
		}
	}

	if( result == 0 ) {
		cout << "Training classifier..." << endl;
		if( !classifier->Train(train[0], trainLabel, trainSet.GetNumObjs(), dims) ) {
			cerr << "Classifier training failed" << endl;
			result = -32;
		}
	}

	if( result == 0 ) {
		cout << "Applying classifier..." << endl;
		if( ! classifier->ScoreBatch(test, numTestObjs, dims, predScore) ) {
			cerr << "Applying classifier failed" << endl;
			result = -33;
		}
	}

	if( result == 0 ) {
		// Copy original test file so we can just append the
		// score data
		//
		string cmd = "cp " + testFileName + " " + outFileName;
		result = system(cmd.c_str());
	}

	if( result == 0 ) {
		hid_t		fileId;
		hsize_t		dims[2];
		herr_t		status;

		fileId = H5Fopen(outFileName.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
		if( fileId < 0 ) {
			cerr << "Unable to open: " << outFileName << endl;
			result = -34;
		}

		if( result == 0 ) {
			dims[0] = numTestObjs;
			dims[1] = 1;

			status = H5LTmake_dataset(fileId, "/pred_score", 2, dims,
										H5T_NATIVE_FLOAT, predScore);
			if( status < 0 ) {
				cerr << "Unable to write score data" << endl;
				result = -35;
			}
		}

		if( fileId >= 0 ) {
			H5Fclose(fileId);
		}
	}

	if( predScore )
		free(predScore);

	return result;
}


int main(int argc, char *argv[])
{
	int 			result = 0;
	gengetopt_args_info	args;

	if( cmdline_parser(argc, argv, &args) != 0 ) {
		exit(-1);
	}

	gLogger = new EvtLogger();

	MData	testSet, trainSet;
	string 	classType = args.classifier_arg,
			trainFile = args.train_file_arg,
			testFile = args.test_file_arg,
			command = args.command_arg,
			outFileName = args.output_file_arg,
			imlabelDir,
			slide;
	int startx = args.startx_arg,
		starty = args.starty_arg,
		width = args.width_arg,
		height = args.height_arg;
	Classifier		*classifier = NULL;

	if( command.compare("map") == 0 ) {
		if( args.slide_given == 0 ) {
			cerr << "Must specify slide name (-s) for the map command" <<  endl;
			exit(-1);
		} else {
			slide = args.slide_arg;
		}
	}

	// if( command.compare("mask") == 0 ) {
	// 	if( args.slide_given == 0 ) {
	// 		cerr << "Must specify slide name (-s) for the map command" <<  endl;
	// 		exit(-1);
	// 	} else {
	// 		slide = args.slide_arg;
	// 	}
	// }

	if( command.compare("maskregion") == 0 ) {
		if( args.slide_given == 0 ) {
			cerr << "Must specify slide name (-s) for the map command" <<  endl;
			exit(-1);
		} else {
			slide = args.slide_arg;
		}
	}

	if( args.labeldir_given == 0 ) {
		imlabelDir = "None";
	}
	else {
		imlabelDir = args.labeldir_arg;
	}


 	if( trainSet.Load(trainFile) && testSet.Load(testFile) ) {
		result = Renormalize(trainSet, testSet);
	} else {
		cerr << "Unable to load datafiles" << endl;
		result = -3;
	}

	if( result == 0 ) {
		if( classType.compare("svm") == 0 ) {
			classifier = new OCVBinarySVM();
		} else if( classType.compare("randForest") == 0 ) {
			classifier = new OCVBinaryRF();
		} else {
			cerr << "Unknown classifier type" << endl;
			result = -2;
		}
	}

	if( result == 0 ) {
		if( command.compare("count") == 0 ) {
			result = ClassifySlides(trainSet, testSet, classifier, testFile, outFileName);
		} else if( command.compare("roc") == 0 ) {
			result = CalcROC(trainSet, testSet, classifier, testFile, outFileName);
		} else if( command.compare("map") == 0 ) {
			result = GenerateMap(trainSet, testSet, classifier, slide, outFileName);
		} else if( command.compare("apply") == 0 ) {
			result = ApplyClassifier(trainSet, testSet, classifier, testFile, outFileName);
		} else if( command.compare("mask") == 0 ) {
			result = GenerateMaskRegions(trainSet, testSet, classifier, outFileName);
			// result = GenerateLabelRegions(trainSet, testSet, classifier, outFileName);
		} else if( command.compare("maskregion") == 0 ) {
			result = GenerateMaskRegion(trainSet, testSet, classifier, testFile, slide, startx, starty, width, height, outFileName);
			// result = GenerateLabelRegion(trainSet, testSet, classifier, testFile, slide, startx, starty, width, height, outFileName);
			// result = GenerateLabelRegion(testSet, testFile, slide, startx, starty, width, height, outFileName);
		}
	}

	if( classifier )
		delete classifier;

	return result;
}
