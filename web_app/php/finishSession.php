<?php

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

	require 'hostspecs.php';
	require '../db/logging.php';
	session_start();
	
	$prog = true;
	
	// Send finalize command to al server to save the training set and
	// get the pertinent info to save in the database
	//
	$end_data =  array( "command" => "finalize", 
			 	   "uid" => $_SESSION['uid']);
	$end_data = json_encode($end_data);
			 	   
	$addr = gethostbyname($host);
	set_time_limit(0);
	
	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	if( $socket === false ) {
		log_error("socket_create failed: ".socket_strerror(socket_last_error()));
		$prog = false;
	}
	
	if( $prog ) {
		$result = socket_connect($socket, $addr, $port);
		if( !$result ) {
			log_error("socket_connect failed: ".socket_strerror(socket_last_error()));
			$prog = false;
		}
	}
	
	if( $prog ) {
		socket_write($socket, $end_data, strlen($end_data));

		$response = socket_read($socket, 8192);
		$additional = socket_read($socket, 8192);
		while( $additional != false ) {
			$response = $response.$additional;
			$additional = socket_read($socket, 8192);
		}
		socket_close($socket);
		$classification = json_decode($classification, true);
	
		socket_close($socket);

		$response = json_decode($response, true);


		$dbConn = guestConnect();
		// Get dataset ID
		if( $result = mysqli_query($dbConn, 'SELECT id from datasets where name="'.$_SESSION['dataset'].'"') ) {

			$array = mysqli_fetch_row($result);
			$datasetId = $array[0];
			mysqli_free_result($result);
		}
	
		if( trim($response['filename']) === "" || !isset($response['filename']) ) {
			$response['filename'] = "lost filename";
		}
		
		// Add classifier to database
		//
		$sql = 'INSERT INTO training_sets (name, type, dataset_id, iterations, filename)';
		$sql = $sql.' VALUES("'.$_SESSION['classifier'].'", "binary", '.$datasetId;
		$sql = $sql.', '.$response['iterations'].', "'.$response['filename'].'")';
	
		$status = mysqli_query($dbConn, $sql);
		$trainingSetId = $dbConn->insert_id;
		if( $status == FALSE ) {
			log_error("Unable to insert classifier into database ".mysqli_error($dbConn));
			log_error("Offending SQL: ".$sql);
			$prog = false;
		}
	}
	
	if( $prog ) {
		// Add classes to the database
		//		!!!! Assuming binary for now !!!!
		//
		$sql = 'INSERT INTO classes (name, training_set_id, color, label)';
		$sql = $sql.'VALUES("'.$_SESSION['posClass'].'",'.$trainingSetId.', "green", 1)';
		$status = mysqli_query($dbConn, $sql);
		$posId = $dbConn->insert_id;
		if( $status == FALSE ) {
			log_error("Unable to insert pos class into database ".mysqli_error($dbConn));
			log_error("Offending SQL: ".$sql);
			$prog = false;
		}
	}
	
	if( $prog ) {
		$sql = 'INSERT INTO classes (name, training_set_id, color, label)';
		$sql = $sql.'VALUES("'.$_SESSION['negClass'].'",'.$trainingSetId.', "red", -1)';
		$status = mysqli_query($dbConn, $sql);
		$negId = $dbConn->insert_id;
		if( $status == FALSE ) {
			log_error("Unable to insert neg class into database ".mysqli_error($dbConn) );
			log_error("Offending SQL: ".$sql);
			$prog = false;
		}
	}
	
	if( $prog ) {
		// Add samples to the database
		//
		for($i = 0, $len = count($response['samples']); $i < $len; ++$i) {

			if( $response['samples'][$i]['label'] === 1 ) {
				$classId = $posId;
			} else {
				$classId = $negId;
			}

			$sql = 'INSERT INTO training_objs (training_set_id, cell_id, iteration, class_id)';
			$sql = $sql.'VALUES('.$trainingSetId.','.$response['samples'][$i]['id'].', '.$response['samples'][$i]['iteration'].','.$classId.')';
			mysqli_query($dbConn, $sql);
		}
		mysqli_close($dbConn);
	
		write_log("INFO", "Session ".$_SESSION['classifier']." finished, Training set saved to: ".$response['filename']);
	
		echo "PASS";
	}
	
	if( $prog == false ) {
		echo "FAIL";
	}
		
	// Cleanup session variables
	//
	session_destroy();	
?>
