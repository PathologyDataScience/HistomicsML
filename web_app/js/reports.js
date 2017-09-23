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
//
//	Initialization
//
//

var application = "";

$(function() {

	application = $_GET("application");

	var	datasetslideSummary = $("#datasetSel"), trainsetSel = $("#trainsetSel"),
		downloadsetSel = $("#downloadsetSel"), datasetpredictDataset = $("#applyDatasetSel"),
		datasetpredictSlide = $('#datasetMapSel');

	document.getElementById("index").setAttribute("href","index.html");
	document.getElementById("home").setAttribute("href","index_home.html?application="+application);
	document.getElementById("viewer").setAttribute("href","viewer.html?application="+application);
	document.getElementById("nav_reports").setAttribute("href","reports.html?application="+application);
	document.getElementById("nav_data").setAttribute("href","data.html?application="+application);
	document.getElementById("nav_validation").setAttribute("href","validation.html?application="+application);

	// Populate Dataset dropdown
	//
	$.ajax({
		type: "POST",
		url: "db/getdatasets.php",
		data: { application: application },
		dataType: "json",
		success: function(data) {

			var curDataset = data[0];

			for( var item in data ) {
				datasetslideSummary.append(new Option(data[item][0], data[item][1]));
				datasetpredictSlide.append(new Option(data[item][0], data[item][1]));
				datasetpredictDataset.append(new Option(data[item][0], data[item][1]));
			}
			updateTrainSetsforSlideSummary(curDataset[0]);
			updateTrainSets(curDataset[0]);
			updateSlideList();
			updateTrainSetsforPredictDataset(curDataset[0]);

		}
	});

	// Populate training set dropdown
	//
	$.ajax({
		url: "db/getTrainingSets.php",
		data: "",
		dataType: "json",
		success: function(data) {
			for( var item in data ) {
				downloadsetSel.append(new Option(data[item][0], data[item][1]));
			}
		}
	});

	// Need to montior changes for the map score select controls. Slide image
	//	size is dependant on these.
	//
	datasetslideSummary.change(updateslideSummary);
	datasetpredictSlide.change(updatepredictSlide);
	$("#slideMapSel").change(updateSlideSize);
	datasetpredictDataset.change(updatepredictDataset);

});


function updateslideSummary() {
	var sel = document.getElementById('datasetSel'),
			  dataset = sel.options[sel.selectedIndex].label;
	updateTrainSetsforSlideSummary(dataset);
}

function updatepredictSlide() {
	var sel = document.getElementById('datasetMapSel'),
			  dataset = sel.options[sel.selectedIndex].label;
	updateTrainSets(dataset);
	updateSlideList();
}

function updatepredictDataset() {
	var sel = document.getElementById('applyDatasetSel'),
			  dataset = sel.options[sel.selectedIndex].label;
	updateTrainSetsforPredictDataset(dataset);
}



function updateTrainSetsforSlideSummary(dataSet) {

	$.ajax({
		type: "POST",
		url: "db/getTrainsetForDataset.php",
		data: { dataset: dataSet },
		dataType: "json",
		success: function(data) {

			var	reloadTrainSel = $("#trainsetSel");
			$("#trainsetSel").empty();

			for( var item in data.trainingSets ) {
				reloadTrainSel.append(new Option(data.trainingSets[item], data.trainingSets[item]));
			}
		}
	});

}



//
//	Updates the list of available slides for the current dataset
//
function updateSlideList() {

	var	dataset = datasetMapSel.options[datasetMapSel.selectedIndex].label;

	// Get the list of slides for the current dataset
	$.ajax({
		type: "POST",
		url: "db/getslides.php",
		data: { dataset: dataset },
		dataType: "json",
		success: function(data) {

			$('#slideMapSel').empty();
			// Add the slides we have segmentation boundaries for to the dropdown
			// selector
			for( var item in data['slides'] ) {
				$('#slideMapSel').append(new Option(data['slides'][item], data['slides'][item]));
			}
			updateSlideSize();
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

			var	reloadTrainSel = $("#trainsetMapSel");
			$("#trainsetMapSel").empty();

			for( var item in data.trainingSets ) {
				reloadTrainSel.append(new Option(data.trainingSets[item], data.trainingSets[item]));
			}
		}
	});

}



function updateTrainSetsforPredictDataset(dataSet) {

	$.ajax({
		type: "POST",
		url: "db/getTrainsetForDataset.php",
		data: { dataset: dataSet },
		dataType: "json",
		success: function(data) {

			var	reloadTrainSel = $("#applyTrainsetSel");
			$("#applyTrainsetSel").empty();

			for( var item in data.trainingSets ) {
				reloadTrainSel.append(new Option(data.trainingSets[item], data.trainingSets[item]));
			}
		}
	});

}




function updateSlideSize() {

	var	slide = slideMapSel.options[slideMapSel.selectedIndex].label;

	$.ajax({
		type: "POST",
		url: "db/getImgSize.php",
		data: { slide: slide },
		dataType: "json",
		success: function(data) {

			document.getElementById('imgSize').innerHTML = data[0]+" x "+data[1] +" pixels ";

			if( data[2] == 1 ) {
				document.getElementById('imgScale').innerHTML = "20x";
			} else if( data[2] == 2 ) {
				document.getElementById('imgScale').innerHTML = "40x";
			} else {
				document.getElementById('imgScale').innerHTML = "???";
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
