<?php

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
	require '../db/logging.php';

	$trainSet = '../trainingsets/'.$_POST['validateTrainSet'].'.h5';
	$testSet = '../trainingsets/'.$_POST['validateTestSet'].'.h5';

	// Extract just the file name of the training set
	$parts = explode("/",$_POST['validateTrainSet']);
	$ele = count($parts);
	if( $ele > 1 ) {
		$trainName = $parts[$ele - 1];
	} else {
		$trainName = $parts[0];
	}

	$parts = explode("/",$_POST['validateTestSet']);
	$ele = count($parts);
	if( $ele > 1 ) {
		$testName = $parts[$ele - 1];
	} else {
		$testName = $parts[0];
	}

	$outFile = '../trainingsets/tmp/'.$trainName.'_'.$testName.'.csv';

	if( file_exists($testSet) && file_exists($trainSet)) {
		$cmd = 'validate -t '.$trainSet.' -f '.$testSet.' -m roc -c randForest -o '.$outFile;

		exec($cmd, $output, $resultVal);

		write_log("INFO","Return value: ".(int)$resultVal);

		if( $resultVal == 0 ) {

			header('Content-Description: File Transfer');
			header('Content-Type: application/octet-stream');
			header('Content-Disposition: attachement; filename='.basename($outFile));
			header('Expires: 0');
			header('Cache-Control: must-revalidate');
			header('Pragma: public');
			header('Content-Length: '. filesize($outFile));
			readfile($outFile);
			exit;
		} else {
			log_error("Can't find ".$trainSet." or ".$testSet);
		}
	}
?>
