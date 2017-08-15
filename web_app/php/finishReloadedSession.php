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

		$response = json_decode($response, true);

		// This training set has been saved before, so only the samples need
		// to be processed. Some may have been updated, others may me new.
		//
		$dbConn = guestConnect();
		// Get dataset ID
		if( $result = mysqli_query($dbConn, 'SELECT id from datasets where name="'.$_SESSION['dataset'].'"') ) {

			$array = mysqli_fetch_row($result);
			$datasetId = $array[0];
			mysqli_free_result($result);
		}
	}
	
	if( $prog ) {
		$sql = 'SELECT id from training_sets WHERE name="'.$_SESSION['classifier'].'"';
		if( $result =  mysqli_query($dbConn, $sql) ) {
			$trainingSetId = mysqli_fetch_row($result)[0];
			mysqli_free_result($result);			
		}
	}

	if( $prog ) {
		// Get class id's 
		$sql = 'SELECT id from classes WHERE training_set_id='.$trainingSetId.' AND label = 1';
		if( $result =  mysqli_query($dbConn, $sql) ) {
			$posId = mysqli_fetch_row($result)[0];
			mysqli_free_result($result);			
		}

		$sql = 'SELECT id from classes WHERE training_set_id='.$trainingSetId.' AND label = -1';
		if( $result =  mysqli_query($dbConn, $sql) ) {
			$negId = mysqli_fetch_row($result)[0];
			mysqli_free_result($result);			
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

			// Check if object already exists.
			$sql = 'SELECT cell_id from training_objs WHERE training_set_id='.$trainingSetId
				.' AND cell_id='.$response['samples'][$i]['id'];

			if( $result =  mysqli_query($dbConn, $sql) ) {
				$objId = mysqli_fetch_row($result)[0];
				mysqli_free_result($result);			
			}

			if( $objId == NULL ) {
				$sql = 'INSERT INTO training_objs (training_set_id, cell_id, iteration, class_id)'
				      .'VALUES('.$trainingSetId.','.$response['samples'][$i]['id'].', '
					  .$response['samples'][$i]['iteration'].','.$classId.')';
				mysqli_query($dbConn, $sql);
			} else {
				$sql = 'UPDATE training_objs SET class_id='.$classId
					   .' WHERE cell_id='.$objId.' AND training_set_id='.$trainingSetId;
				if( !mysqli_query($dbConn, $sql) ) {
					log_error("Error updating record: ". mysqli_error($dbConn));
				}				
			}
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
