<?php

//
//	Copyright (c) 2014-2016, Emory University
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
	require 'hostspecs.php';			// Defines $host and $port
	require '../db/logging.php';		// Also includes connect.php
	require 'serverComm.php';

	session_start();
	
	$prog = true;

	$end_data =  array( "command" => "pickerSave", 
			 	   "uid" => $_SESSION['uid']);
	$end_data = json_encode($end_data);
			 	   
	$socket = connect_server($host, $port);
	if( $socket === false ) {
		$prog = false;
	} else {
		$response = command_server($end_data, $socket);
		if( $response === false ) {
			$prog = false;
		} else {
			$response = json_decode($response, true);
		}
	}

	if( $prog ) {
		$dbconn = guestConnect();
		$sql = 'SELECT id FROM datasets WHERE name="'.$_SESSION['dataset'].'"';

		if( $result = mysqli_query($dbconn, $sql) ) {
			$array = mysqli_fetch_row($result);
			$datasetId = $array[0];
			mysqli_free_result($result);
		} else {
			log_error("Unable to get dataset id from database");
			$prog = false;
		}
	}

	if( $prog ) {
		$sql = 'INSERT INTO test_sets (name, dataset_id, filename, pos_name, neg_name) ';
		$sql = $sql.'VALUES("'.$_SESSION['classifier'].'", '.$datasetId.', ';
		$sql = $sql.'"'.$response['filename'].'", "'.$response['posClass'].'", "'.$response['negClass'].'")';

		$status = mysqli_query($dbconn, $sql);
		if( $status === FALSE ) {
			log_error("Unable to save test set to database");
			$prog = false;
		}
	}

	if( $prog === FALSE ) {
		$response['status'] = "FAIL";
	}

	write_log("INFO", "Picker session ".$_SESSION['classifier']." Finished with status: ".$response['status']);
	echo json_encode($response);

	// Cleanup session variables
	//
	session_unset();
	session_destroy();
?>
