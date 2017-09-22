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
var uid = "";
var classifier = "";
var negClass = "";
var posClass = "";
var curDataSet = "";
var application = "";


//
//	Initialization
//
//
$(function() {

	application = $_GET("application");

	// get session vars
	//
	$.ajax({
		url: "php/getSession.php",
		data: "",
		dataType: "json",
		success: function(data) {

			uid = data['uid'];
			classifier = data['className'];
			posClass = data['posClass'];
			negClass = data['negClass'];
			curDataset = data['dataset'];
			IIPServer = data['IIPServer'];

		}
	});


	document.getElementById("index").setAttribute("href","index.html");
	document.getElementById("home").setAttribute("href","index_home.html?application="+application);
	document.getElementById("viewer").setAttribute("href","viewer.html?application="+application);
	document.getElementById("nav_reports").setAttribute("href","reports.html?application="+application);
	document.getElementById("nav_data").setAttribute("href","data.html?application="+application);
	document.getElementById("nav_validation").setAttribute("href","validation.html?application="+application);

	$("#applicationSel").val(application);
	$("#applicationSelreload").val(application);
	$("#applicationSelvalidate").val(application);

	// Populate Dataset dropdown
	//
	$.ajax({
		type: "POST",
		url: "db/getdatasets.php",
		data: { application: application },
		dataType: "json",
		success: function(data) {

				var	datasetSel = $("#datasetSel"),
					reloadDatasetSel = $("#reloadDatasetSel"),
					validateDatasetSel = $("#validateDatasetSel");
				curDataset = data[0];

			for( var item in data ) {
				datasetSel.append(new Option(data[item][0], data[item][0]));
				reloadDatasetSel.append(new Option(data[item][0], data[item][0]));
				validateTrainDatasetSel.append(new Option(data[item][0], data[item][0]));
				validateDatasetSel.append(new Option(data[item][0], data[item][0]));
			}

			updateTestSets(curDataset[0]);
			updateTrainSets(curDataset[0]);
			updateSetsForValidate(curDataset[0]);
		}
	});
	$('#reloadDatasetSel').change(updateDataSet);
	$('#validateTrainDatasetSel').change(updateDataSetforTrain);
	$('#validateDatasetSel').change(updateDataSetforTest);
});

function updateDataSetforTrain() {

	var sel = document.getElementById('validateTrainDatasetSel'),
			  dataset = sel.options[sel.selectedIndex].label;

	updateTrainSets(dataset);
}

function updateDataSetforTest() {

	var sel = document.getElementById('validateDatasetSel'),
			  dataset = sel.options[sel.selectedIndex].label;

	updateSetsForValidate(dataset);
}

function updateDataSet() {

	var sel = document.getElementById('reloadDatasetSel'),
			  dataset = sel.options[sel.selectedIndex].label;

	updateTestSets(dataset);
}



function updateTestSets(dataset) {

	$.ajax({
		type: "POST",
		url: "db/getTestsetForDataset.php",
		data: { dataset: dataset },
		dataType: "json",
		success: function(data) {

			var reloadTestSel = $("#reloadTestSetSel");

			$("#reloadTestSetSel").empty();

			if( reloadTestSel.length == 0 ) {
				reloadTestSel.classList.toggle("show");
			}

			if( data.testSets.length === 0 ) {
				document.getElementById('reloadPicker').disabled = true;
			} else {
				document.getElementById('reloadPicker').disabled = false;
			}

			for( var item in data.testSets ) {
				reloadTestSel.append(new Option(data.testSets[item], data.testSets[item]));
			}
		},
		error: function(x,s,e) {
			console.log("Error:"+e);
		}
	});

}

function updateSetsForValidate(dataset) {

	$.ajax({
		type: "POST",
		url: "db/getTestsetForDataset.php",
		data: { dataset: dataset },
		dataType: "json",
		success: function(data) {

			var validateTestSel = $("#validateTestsetSel");

			$("#validateTestsetSel").empty();

			if( validateTestSel.length == 0 ) {
				validateTestSel.classList.toggle("show");
			}

			for( var item in data.testSets ) {
				validateTestSel.append(new Option(data.testSets[item], data.testSets[item]));
			}
		},
		error: function(x,s,e) {
			console.log("Error:"+e);
		}
	});
}


function updateTrainSets(dataSet) {

	$.ajax({
		type: "POST",
		url: "db/getTrainsetForDataset.php",
		data: { dataset: dataSet },
		dataType: "json",
		success: function(data) {

			var	reloadTrainSel = $("#validateTrainsetSel");
			$("#validateTrainsetSel").empty();

			for( var item in data.trainingSets ) {
				reloadTrainSel.append(new Option(data.trainingSets[item], data.trainingSets[item]));
			}
		}
	});

}
//
// Retruns the value of the GET request variable specified by name
//
//
function $_GET(name) {
	var match = RegExp('[?&]' + name + '=([^&]*)').exec(window.location.search);
	return match && decodeURIComponent(match[1].replace(/\+/g,' '));
}



function displayProg() {

	$('#progDiag').modal('show');
}
