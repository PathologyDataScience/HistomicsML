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
	require 'serverComm.php';
	require '../db/logging.php';		// Also includes connect.php
	require_once 'hostspecs.php';			// Defines $host and $port


	// Generate a unique id to track this session in the server
	//
	$UID = uniqid("", true);

	// Get the trainingset file from the database
	//
	$dbConn = guestConnect();
	$sql = 'SELECT filename FROM test_sets WHERE name="'.$_POST["testSet"].'"';

	if( $result = mysqli_query($dbConn, $sql) ) {

		$filename = mysqli_fetch_row($result);
		mysqli_free_result($result);
	} else {
		log_error("Unable to get test set from the database");
	}

	$sql = 'SELECT features_file FROM datasets WHERE name="'.$_POST["reloadDataset"].'"';
	if( $result = mysqli_query($dbConn, $sql) ) {

		$featureFile = mysqli_fetch_row($result);
		mysqli_free_result($result);
	} else {
		log_error("Unable to get training set from the database");
	}

	write_log("INFO", "Reloading:  '".$featureFile[0]."', trainset: ".$filename[0]);

	mysqli_close($dbConn);


	// Send init command to AL server
	//
	$init_data =  array( "command" => "pickerReload",
						 "dataset" => $_POST["reloadDataset"],
				 	     "features" => $featureFile[0],
						 "name" => $_POST["testSet"],
					     "testfile" => $filename[0],
			 	   		 "uid" => $UID);

	$init_data = json_encode($init_data);

	$prog = true;

	$socket = connect_server($host, $port);
	if( $socket === false ) {
		$prog = false;
	} else {
		$response = command_server($init_data, $socket);
		if( $response === false ) {
			$prog = false;
		} else {
			$response = json_decode($response, true);
		}
	}

	if( $prog ) {

		if( strcmp($response['result'], "PASS") == 0 ) {

			write_log("INFO", "Session '".$_POST["testSet"]."' reloaded");

			session_start();
			$_SESSION['uid'] = $UID;
			$_SESSION['classifier'] = $_POST["testSet"];
			$_SESSION['dataset'] = $_POST["reloadDataset"];
			$_SESSION['posClass'] = $response['posName'];
			$_SESSION['negClass'] = $response['negName'];
			$_SESSION['reloaded'] = true;
			header("Location: ../picker.html?application=".$_POST["applicationreload"]);
		} else {
			echo "Unable to init session<br>";
		}
	}
?>
