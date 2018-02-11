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
var application = "";
var strlink = "";
var superpixel_size = 0;


//
//	Initialization
//
//
$(function() {

	application = $_GET("application");

	var	datasetSel = $("#datasetSel");


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

			if( uid === null ) {
				$('#nav_select').hide();
				$('#nav_heatmaps').hide();
				$('#nav_review').hide();
				$('#nav_survival').hide();

				document.getElementById("index").setAttribute("href","index.html");

			} else {
				// There's an active session, disable the "start session"
				// form.
				//
				$('#beginSession').attr('disabled', 'true');
				$('#trainset').attr('disabled', 'true');
				$('#datasetSel').attr('disabled', 'true');
				$('#posClass').attr('disabled', 'true');
				$('#negClass').attr('disabled', 'true');
				$('#reloadSession').attr('disabled', 'true');
				$('#reloadDatasetSel').attr('disabled', 'true');
				$('#reloadTrainSetSel').attr('disabled', 'true');

				// No reports while session active
				$('#nav_reports').hide();
				$('#nav_data').hide();
				$('#nav_validation').hide();

				document.getElementById("index").removeAttribute('href');

				// TODO - Populate the text fields with the session values.
				// This way we can see the criteria for the
				// current session
			}

			document.getElementById("viewer").setAttribute("href","viewer.html?application="+application);
			document.getElementById("nav_reports").setAttribute("href","reports.html?application="+application);
			document.getElementById("nav_select").setAttribute("href","grid.html?application="+application);
			document.getElementById("nav_heatmaps").setAttribute("href","heatmaps.html?application="+application);
			document.getElementById("nav_data").setAttribute("href","data.html?application="+application);
			document.getElementById("nav_validation").setAttribute("href","validation.html?application="+application);
			document.getElementById("nav_survival").setAttribute("href","survival.html?application="+application);

			$("#applicationSel").val(application);
			$("#applicationSelreload").val(application);
			//$("#applicationSel").css("background-color", "gray");

		}
	});

	// Populate Dataset dropdown
	//
	$.ajax({
		type: "POST",
		url: "db/getdatasets.php",
		data: { application: application },
		dataType: "json",
		success: function(data) {

			var	reloadDatasetSel = $("#reloadDatasetSel");

			curDataset = data[0];		// Use first dataset initially

			for( var item in data ) {
				datasetSel.append(new Option(data[item][0], data[item][0]));
				reloadDatasetSel.append(new Option(data[item][0], data[item][0]));
			}
			superpixel_size = curDataset[2];
			updateTrainingSets(curDataset[0]);
		}
	});

	$('#reloadTrainSetSel').change(updateTrainingSet);
	$('#reloadDatasetSel').change(updateDataset)
});





function updateDataset() {

	var dataset = reloadDatasetSel.options[reloadDatasetSel.selectedIndex].label;
	updateTrainingSets(dataset);
}





function updateTrainingSets(dataSet) {

	$.ajax({
		type: "POST",
		url: "db/getTrainsetForDataset.php",
		data: { dataset: dataSet },
		dataType: "json",
		success: function(data) {

			var	reloadTrainSel = $("#reloadTrainSetSel");
			$("#reloadTrainSetSel").empty();
			if( reloadTrainSetSel.length == 0 ) {0

				reloadTrainSetSel.classList.toggle("show");
			}

			for( var item in data.trainingSets ) {
				reloadTrainSel.append(new Option(data.trainingSets[item], data.trainingSets[item]));
			}
			updateTrainingsetInfo(data.trainingSets[0]);
		}
	});
}






function updateTrainingSet() {

	var trainSet = reloadTrainSetSel.options[reloadTrainSetSel.selectedIndex].label;

	updateTrainingsetInfo(trainSet);
}





function updateTrainingsetInfo(trainSet) {

	$.ajax({
		type: "POST",
		url: "db/getTrainsetInfo.php",
		data: { trainset: trainSet },
		dataType: "json",
		success: function(data) {

			document.getElementById('reloadNeg').innerHTML = data.labels[0];
			document.getElementById('reloadPos').innerHTML = data.labels[1];
			document.getElementById('reloadIter').innerHTML = data.iterations;
			document.getElementById('reloadNegCount').innerHTML = data.counts[0];
			document.getElementById('reloadPosCount').innerHTML = data.counts[1];
		}
	});
}




function displayProg() {

	$('#progDiag').modal('show');
}




function resetAlServer() {

	console.log("Canceling session");

	$.ajax({
		url: "php/cancelSession.php",
		data: "",
		success: function() {
			window.location = "index_home.html?application="+application;
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
