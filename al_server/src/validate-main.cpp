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
#include <iostream>
#include <set>

#include "ocvsvm.h"
#include "ocvrandforest.h"
#include "data.h"
#include "logger.h"

#include "val-cmdline.h"
#include "base_config.h"


using namespace std;


EvtLogger	*gLogger = NULL;




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
			slide;
	Classifier		*classifier = NULL;

	if( command.compare("map") == 0 ) {
		if( args.slide_given == 0 ) {
			cerr << "Must specify slide name (-s) for the map command" <<  endl;
			exit(-1);
		} else {
			slide = args.slide_arg;
		}
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
		}
	}

	if( classifier )
		delete classifier;

	return result;
}
