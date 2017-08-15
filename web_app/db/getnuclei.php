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

	require 'logging.php';		// Also includes connect.php
	require '../php/hostspecs.php';


	// 	Retrieve a list of nuclei boundaries within the given rectangle
	//	Return as an array of JSON objects, where each object is ordered:
	//
	//		0 - boundary
	//		1 - Id
	//		2 - color


	//	Get the bounding box and slide name passed by the ajax call
	//
	$left = intval($_POST['left']);
	$right = intval($_POST['right']);
	$top = intval($_POST['top']);
	$bottom = intval($_POST['bottom']);
	$slide = $_POST['slide'];
	$trainSet = $_POST['trainset'];
	$application = $_POST['application'];

	//write_log("INFO", "trainSet: ".$trainSet);

	// Get labels for the objects within the viewport
	if( $trainSet != "none" ) {

		if( $trainSet === "PICKER" ) {
			$colors = array('aqua', 'yellow');
		} else {
			$colors = array('lightgrey', 'lime');
		}

		$dataSet = $_POST['dataset'];

		// Send command to al server
		$cmd =  array( "command" => "apply",
					   "uid" => $_POST['uid'],
				 	   "slide" => $slide,
				 	   "xMin" => $left,
				 	   "xMax" => $right,
				 	   "yMin" => $top,
				 	   "yMax" => $bottom
				 	   );

		$cmd = json_encode($cmd);

		$addr = gethostbyname($host);
		set_time_limit(0);

		$socket = socket_create(AF_INET, SOCK_STREAM, 0);
		if( $socket === false ) {
			log_error("socket_create failed:  ". socket_strerror(socket_last_error()));
		}

		$result = socket_connect($socket, $addr, $port);
		if( !$result ) {
			log_error("socket_connect failed: ".socket_strerror(socket_last_error()));
		}

		socket_write($socket, $cmd, strlen($cmd));

		// Get classification results, The al server closes the connection
		// after sending data, so it's safe to loop on the result of
		// socket_read
		//
		$classification = socket_read($socket, 8192);
		$additional = socket_read($socket, 8192);
		while( $additional != false ) {
			$classification = $classification.$additional;
			$additional = socket_read($socket, 8192);
		}
		socket_close($socket);
		$classification = json_decode($classification, true);
	}

  $boundaryTablename = "boundaries";
  if ($application == "region"){
		$boundaryTablename = "sregionboundaries";
	}

	$dbConn = guestConnect();
	//$sql = 'SELECT boundary, id, centroid_x, centroid_y from "'.$boundaryTablename.'" where slide="'.$slide.'" AND centroid_x BETWEEN '.$left.' AND '.$right.' AND centroid_y BETWEEN '.$top.' AND '.$bottom;
	$sql = 'SELECT boundary, id, centroid_x, centroid_y from '.$boundaryTablename.' where slide="'.$slide.'" AND centroid_x BETWEEN '.$left.' AND '.$right.' AND centroid_y BETWEEN '.$top.' AND '.$bottom;

	//write_log("INFO", "sql: ".$sql);

	if( $result = mysqli_query($dbConn, $sql) ) {

		$jsonData = array();
		while( $array = mysqli_fetch_row($result) ) {
			$obj = array();

			if ($application == "cell"){
				$obj[] = $array[1];
				$obj[] = $array[2];
				$obj[] = $array[3];
			} else{
				$obj[] = $array[0];
				$obj[] = $array[1];				
			}

			if( $trainSet != "none" ) {
				$tag = $array[2]."_".$array[3];
				$obj[] = $colors[$classification[$tag]];
			} else {
				// Default to aqua if a classifier is not being applied
				$obj[] = "aqua";
			}

			if ($application == "region"){
				$obj[] = $array[2];
				$obj[] = $array[3];
			}

			$jsonData[] = $obj;
		}
		mysqli_free_result($result);
	}
	mysqli_close($dbConn);

	echo json_encode($jsonData);

?>
