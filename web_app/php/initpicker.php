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

	// Make sure initPicker.php was referenced from the main picker
	// page by checking if 'submit' is empty
	//
	if( empty($_POST['submit']) ) {
		echo "Form was not submitted <br>";
		exit;
	}

	// TODO - Add an alert to indicate what was missing. Should
	// be on the main page


	// Each of the text fields must also be filled in
	//
	if( empty($_POST["testsetname"]) ) {
			// Redirect back to the form
			header("Location:".$_SERVER['HTTP_REFERER']);
			exit;
	}

	if( empty($_POST["posClass"]) ) {
			// Redirect back to the form
			header("Location:".$_SERVER['HTTP_REFERER']);
			exit;
	}

	if( empty($_POST["negClass"]) ) {
			// Redirect back to the form
			header("Location:".$_SERVER['HTTP_REFERER']);
			exit;
	}

	// Generate a unique id to track this session in the server
	//
	$UID = uniqid("", true);

	// Get the dataset file from the database
	//
	$dbConn = guestConnect();
	$sql = 'SELECT features_file FROM datasets WHERE name="'.$_POST["dataset"].'"';

	if( $result = mysqli_query($dbConn, $sql) ) {

		$featureFile = mysqli_fetch_row($result);
		mysqli_free_result($result);
	}
	mysqli_close($dbConn);


	// Send init command to AL server
	//
	$init_data =  array("command" => "pickerInit",
						"name" => $_POST["testsetname"],
						"dataset" => $_POST["dataset"],
						"features" => $featureFile[0],
						"negClass" => $_POST["negClass"],
						"posClass" => $_POST["posClass"],
						"uid" => $UID);

	$init_data = json_encode($init_data);

	require 'hostspecs.php';
	//
	//	$host and $port are defined in hostspecs.php
	//
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

	socket_write($socket, $init_data, strlen($init_data));
	$response = socket_read($socket, 10);
	socket_close($socket);

	if( strcmp($response, "PASS") == 0 ) {
		write_log("INFO", "Picker session '".$_POST["testsetname"]."' started");

		session_start();
		$_SESSION['uid'] = $UID;
		$_SESSION['posClass'] = $_POST["posClass"];
		$_SESSION['negClass'] = $_POST["negClass"];
		$_SESSION['classifier'] = $_POST["testsetname"];	// Reusing className for test set name
		$_SESSION['dataset'] = $_POST["dataset"];
		$_SESSION['reloaded'] = false;
		$_SESSION['iteration'] = 0;
		header("Location: ../picker.html?application=".$_POST['application']);
	} else {

		echo "Unable to init session<br>";
	}
?>
