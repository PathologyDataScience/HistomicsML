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

	require 'hostspecs.php';
	require '../db/logging.php';

	session_start();

	$sel_data =  array( "command" => "select",
	  			 	    "uid" => $_SESSION['uid'],
	  			 	    "iteration" => $_SESSION['iteration']);

	$sel_data = json_encode($sel_data);

	$addr = gethostbyname($host);
	set_time_limit(0);

	$socket = socket_create(AF_INET, SOCK_STREAM, 0);
	if( $socket === false ) {
		echo "socket_create failed:  ". socket_strerror(socket_last_error()) . "<br>";
	}

	$result = socket_connect($socket, $addr, $port);
	if( !$result ) {
		echo "socket_connect failed: ".socket_strerror(socket_last_error()) . "<br>";
	}

	socket_write($socket, $sel_data, strlen($sel_data));
	$response = socket_read($socket, (100 * 1024));
	socket_close($socket);

	// Now get the max X & Y from the database for the slide of the samples
	//
	$dbConn = guestConnect();
	$response = json_decode($response, true);


	for($i = 0, $len = count($response['samples']); $i < $len; ++$i) {

		$response['samples'][$i]['centX'] = round($response['samples'][$i]['centX'], 1);
		$response['samples'][$i]['centY'] = round($response['samples'][$i]['centY'], 1);


		// get slide dimensions for the sample
		$sql = 'SELECT x_size, y_size, pyramid_path, scale FROM slides WHERE name="'.$response['samples'][$i]['slide'].'"';
		if( $result = mysqli_query($dbConn, $sql) ) {
			$row = mysqli_fetch_row($result);

			$response['samples'][$i]['maxX'] = intval($row[0]);
			$response['samples'][$i]['maxY'] = intval($row[1]);
			$response['samples'][$i]['path'] = $row[2];
			$response['samples'][$i]['scale'] = intval($row[3]);

			mysqli_free_result($result);
		}

		// Get database id for the sample
		$application = $_POST['application'];
		$boundaryTablename = "boundaries";
		if ($application == "region"){
			$boundaryTablename = "sregionboundaries";
		}

		$sql = 'SELECT id, boundary FROM '.$boundaryTablename.' WHERE slide="'.$response['samples'][$i]['slide'].'"';
		$sql = $sql.' AND centroid_x='.$response['samples'][$i]['centX'].' and centroid_y='.$response['samples'][$i]['centY'];

		if( $result = mysqli_query($dbConn, $sql) ) {
			$array = mysqli_fetch_row($result);

			$response['samples'][$i]['id'] = intval($array[0]);
			$response['samples'][$i]['boundary'] = $array[1];
			mysqli_free_result($result);
		}
	}
	$_SESSION['iteration'] = $response['iteration'];

	mysqli_close($dbConn);
	$response = json_encode($response);

	echo $response;
?>
