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
var annoGrpTransformFunc;
var IIPServer="";
var slideCnt = 0;
var curSlide = "";
var curDataset = "";
var curClassifier = "none";

var viewer = null;
var imgHelper = null, osdCanvas = null, viewerHook = null;
var overlayHidden = false, selectMode = false, segDisplayOn = false;
var olDiv = null;
var lastScaleFactor = 0;
var pyramids, trainingSets;
var clickCount = 0;

// The following only needed for active sessions
var uid = null, negClass = "", posClass = "";

var boundsLeft = 0, boundsRight = 0, boundsTop = 0, boundsBottom = 0;
var	panned = false;
var	pannedX, pannedY;

var fixes = {iteration: 0, accuracy: 0, samples: []};

var heatmapLoaded = false;
var slideReq = null;
var uncertMin = 0.0, uncertMax = 0.0, classMin = 0.0, classMax = 0.0;

var classifierSession = false;

// check if the screen is panned or not. Defalut value is false
var ispannedXY = false;
var application = "";
//
//	Initialization
//
//		Get a list of available slides from the database
//		Populate the selection and classifier dropdowns
//		load the first slide
//		Register event handlers
//
$(function() {

	slideReq = $_GET('slide');
	// gets x and y positions
	pannedX = $_GET('x_pos');
	pannedY = $_GET('y_pos');

	application = $_GET("application");

	document.getElementById("home").setAttribute("href","index_home.html?application="+application);
	document.getElementById("nav_select").setAttribute("href","grid.html?application="+application);
	document.getElementById("viewer").setAttribute("href","viewer.html?application="+application);
	document.getElementById("nav_review").setAttribute("href","review.html?application="+application);
	document.getElementById("nav_heatmaps").setAttribute("href","heatmaps.html?application="+application);
	document.getElementById("nav_reports").setAttribute("href","reports.html?application="+application);
	document.getElementById("nav_data").setAttribute("href","data.html?application="+application);
	document.getElementById("nav_validation").setAttribute("href","validation.html?application="+application);

	// Create the slide zoomer, update slide count etc...
	// We will load the tile pyramid after the slide list is loaded
	//
	viewer = new OpenSeadragon.Viewer({ showNavigator: true, id: "image_zoomer", prefixUrl: "images/", animationTime: 0.1});
	imgHelper = viewer.activateImagingHelper({onImageViewChanged: onImageViewChanged});
    viewerHook = viewer.addViewerInputHook({ hooks: [
                    {tracker: 'viewer', handler: 'clickHandler', hookHandler: onMouseClick}
            ]});

	annoGrpTransformFunc = ko.computed(function() {
										return 'translate(' + svgOverlayVM.annoGrpTranslateX() +
										', ' + svgOverlayVM.annoGrpTranslateY() +
										') scale(' + svgOverlayVM.annoGrpScale() + ')';
									}, this);

	//
	// Image handlers
	//
	viewer.addHandler('open', function(event) {
		osdCanvas = $(viewer.canvas);
		statusObj.haveImage(true);

		osdCanvas.on('mouseenter.osdimaginghelper', onMouseEnter);
		osdCanvas.on('mousemove.osdimaginghelper', onMouseMove);
		osdCanvas.on('mouseleave.osdimaginghelper', onMouseLeave);

		statusObj.imgWidth(imgHelper.imgWidth);
		statusObj.imgHeight(imgHelper.imgHeight);
		statusObj.imgAspectRatio(imgHelper.imgAspectRatio);
		statusObj.scaleFactor(imgHelper.getZoomFactor());
		// check if the location of x and y is validated or not
		reviewCheck();
	});



	viewer.addHandler('close', function(event) {
		statusObj.haveImage(false);

        osdCanvas.off('mouseenter.osdimaginghelper', onMouseEnter);
        osdCanvas.off('mousemove.osdimaginghelper', onMouseMove);
        osdCanvas.off('mouseleave.osdimaginghelper', onMouseLeave);

		osdCanvas = null;
	});


	viewer.addHandler('animation-finish', function(event) {

		if( segDisplayOn ) {

			if( statusObj.scaleFactor() > 0.5 ) {

				// Zoomed in, show boundaries hide heatmap
				$('#anno').show();
				$('#heatmapGrp').hide();

				var centerX = statusObj.dataportLeft() +
							  ((statusObj.dataportRight() - statusObj.dataportLeft()) / 2);
				var centerY = statusObj.dataportTop() +
							  ((statusObj.dataportBottom() - statusObj.dataportTop()) / 2);

				if( centerX < boundsLeft || centerX > boundsRight ||
					centerY < boundsTop || centerY > boundsBottom ) {

					// Only update boundaries if we've panned far enough.
					updateSeg();
				}

			} else {

				if (application != "region"){
					updateSeg();
				}

				// Zoomed out, hide boundaries, show heatmap
				$('#anno').hide();
				$('#heatmapGrp').show();

				// Reset bounds to allow boundaries to be drawn when
				// zooming in from a heatmap.
				boundsLeft = boundsRight = boundsTop = boundsBottom = 0;
			}
		}
	});

	// get slide host info
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
			IIPServer = data['IIPServer'];
			curDataset = data['dataset'];

			// Don't display the legend until a classifier is selected
			$('#legend').hide();

			if( uid === null ) {
				// No active session, don;t allow navigation to select & visualize
				$('#nav_select').hide();
				$('#nav_heatmaps').hide();
				// the review section also should be hided
				$('#nav_review').hide();

				document.getElementById("index").setAttribute("href","index.html");

			} else {
				// Active session, dataset selection not allowed
				document.getElementById('dataset_sel').disabled = true

				// No report generation during active session
				$('#nav_reports').hide();
				$('#nav_data').hide();
				$('#nav_validation').hide();
			}

			if( curClassifier === "none" ) {
				$('#retrainInfo').hide();
			}

			// Slide list and classifier list will also be updated by this call
			updateDatasetList();
		}
	});


	// Set the update handlers for the selectors
	$("#slide_sel").change(updateSlide);
	$("#dataset_sel").change(updateDataset);
	//$("#classifier_sel").change(updateClassifier);

	// Set update handler for the heatmap radio buttons
	$('input[name=heatmapOption]').change(updateHeatmap);

  // Set filter for numeric input
	$("#x_pos").keydown(filter);
	$("#y_pos").keydown(filter);

	$('#SelClassifier').hide();

});

//
// a check function
// check if the location is validated or not
//

function reviewCheck(){
	if( pannedX === null || pannedY === null ) {
		ispannedXY = false;
	}
	else{
		ispannedXY = true;
		$("#x_pos").val(pannedX);
		$("#y_pos").val(pannedY);
		$("#btn_Go" ).click();
	}
}


function cleanupClassifierSession() {

	$.ajax({
		url: "php/endClassifier.php",
		data: ""
	});
}





// Filter keystrokes for numeric input
function filter(event) {

	// Allow backspace, delete, tab, escape, enter and .
	if( $.inArray(event.keyCode, [46, 8, 9, 27, 13, 110, 190]) !== -1 ||
		// Allow Ctrl-A
	   (event.keyCode == 65 && event.ctrlKey === true) ||
		// Allow Ctrl-C
	   (event.keyCode == 67	&& event.ctrlKey === true) ||
		// Allow Ctrl-X
	   (event.keyCode == 88	&& event.ctrlKey === true) ||
		// Allow home, end, left and right
	   (event.keyCode >= 35	&& event.keyCode <= 39) ) {

			return;
	}

	// Don't allow if not a number
	if( (event.shiftKey || event.keyCode < 48 || event.keyCode > 57) &&
		(event.keyCode < 96 || event.keyCode > 105) ) {

			event.preventDefault();
	}
}



//
//	Get the url for the slide pyramid and set the viewer to display it
//
//
function updatePyramid() {

	slide = "";
	panned = false;
	heatmapLoaded = false;

	// Zoomer needs '.dzi' appended to the end of the filename
	pyramid = "DeepZoom="+pyramids[$('#slide_sel').prop('selectedIndex')]+".dzi";
	viewer.open(IIPServer + pyramid);
}


//
//	Updates the dataset selector
//
function updateDatasetList() {
	var	datasetSel = $("#dataset_sel");

	// Get a list of datasets
	$.ajax({
		type: "POST",
		url: "db/getdatasets.php",
		data: { application: application },
		dataType: "json",
		success: function(data) {

			for( var item in data ) {
				datasetSel.append(new Option(data[item][0], data[item][0]));
			}

			if( curDataset === null ) {
				curDataset = data[0][0];		// Use first dataset initially
			} else {
				datasetSel.val(curDataset);
			}

			// Need to update the slide list since we set the default slide
			updateSlideList();

			// Classifier list needs the current dataset
			updateClassifierList();
		}
	});
}





//
//	Updates the list of available slides for the current dataset
//
function updateSlideList() {
	var slideSel = $("#slide_sel");
	var slideCntTxt = $("#count_patient");

	// Get the list of slides for the current dataset
	$.ajax({
		type: "POST",
		url: "db/getslides.php",
		data: { dataset: curDataset },
		dataType: "json",
		success: function(data) {

			var index = 0;

			pyramids = data['paths'];
			if( slideReq === null ) {
				curSlide = String(data['slides'][0]);		// Start with the first slide in the list
			} else {
				curSlide = slideReq;
			}

			slideCnt = Object.keys(data['slides']).length;;
			slideCntTxt.text(slideCnt);

			slideSel.empty();
			// Add the slides we have segmentation boundaries for to the dropdown
			// selector
			for( var item in data['slides'] ) {

				if( slideReq != null && slideReq == data['slides'][item] ) {
					index = item;
				}
				slideSel.append(new Option(data['slides'][item], data['slides'][item]));
			}

			if( index != 0 ) {
				$('#slide_sel').prop('selectedIndex', index);
			}

			// Get the slide pyrimaid and display
			updatePyramid();
		}
	});
}





//
//	Updates the classifier selector
//
function updateClassifierList() {
	var classSel = $("#classifier_sel");

	classSel.empty();

	if( uid === null ) {
		classSel.append(new Option('----------------', 'none'));
	} else {
		classSel.append(new Option('Current', 'current'));
	}

	updateClassifier();

	/*
	// First selection should be none
	classSel.append(new Option('----------------', 'none'));

	if( uid === null ) {
		$.ajax({
			type: "POST",
			url: "db/getTrainsetForDataset.php",
			data: { dataset: curDataset },
			dataType: "json",
			success: function(data) {

				trainingSets = data;
				for( var item in data['trainingSets'] ) {
					classSel.append(new Option(data['trainingSets'][item], data['trainingSets'][item]));
				}
			}
		});

	} else {
		classSel.append(new Option('Current', 'current'));
	}
	*/
}




//
//	A new slide has been selected from the drop-down menu, update the
// 	slide zoomer.
//
//
function updateSlide() {
	curSlide = $('#slide_sel').val();

	fixes['samples'] = [];
	$('#retrainBtn').attr('disabled', 'disabled');
	updatePyramid();

	if( segDisplayOn ) {

		// Clear heatmap if displayed
		var heatmapGrp = document.getElementById('heatmapGrp');

		if( heatmapGrp != null ) {
			heatmapGrp.parentNode.removeChild(heatmapGrp);
		}

		if (application == "region"){
			updateSlideSeg();
		} else{
			updateSeg();
		}
	}
}

//
//	A new seg should be applied to the slide updated.
//
//
function updateSlideSeg() {

	var ele, segGrp, annoGrp;

	var left, right, top, bottom, width, height;

	// Grab nuclei a viewport width surrounding the current viewport
	//
	width = statusObj.dataportRight() - statusObj.dataportLeft();
	height = statusObj.dataportBottom() - statusObj.dataportTop();

	left = (statusObj.dataportLeft() - width > 0) ?	statusObj.dataportLeft() - width : 0;
	right = statusObj.dataportRight() + width;
	top = (statusObj.dataportTop() - height > 0) ?	statusObj.dataportTop() - height : 0;
	bottom = statusObj.dataportBottom() + height;


	var class_sel = document.getElementById('classifier_sel');

    $.ajax({
		type: "POST",
     	 	url: "db/getnuclei.php",
     	 	dataType: "json",
		data: { uid:	uid,
				slide: 	curSlide,
				left:	left,
				right:	right,
				top:	top,
				bottom:	bottom,
				dataset: curDataset,
				trainset: curClassifier,
				application: application
		},

		success: function(data) {

				segGrp = document.getElementById('segGrp');
				annoGrp = document.getElementById('anno');

				// Save current viewport location
				boundsLeft = statusObj.dataportLeft();
				boundsRight = statusObj.dataportRight();
				boundsTop = statusObj.dataportTop();
				boundsBottom = statusObj.dataportBottom();

				// If group exists, delete it
				if( segGrp != null ) {
					segGrp.parentNode.removeChild(segGrp);
				}

				// Create segment group
        segGrp = document.createElementNS("http://www.w3.org/2000/svg", "g");
        segGrp.setAttribute('id', 'segGrp');
        annoGrp.appendChild(segGrp);


				for( cell in data ) {
					ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");

					ele.setAttribute('points', data[cell][0]);
					ele.setAttribute('id', 'N' + data[cell][1]);
					ele.setAttribute('stroke', 'aqua');
					ele.setAttribute('stroke-width', 4);
					ele.setAttribute("stroke-dasharray", "5,5");
					ele.setAttribute('fill', data[cell][2]);
					ele.setAttribute("fill-opacity", "0.2");
					//ele.setAttribute("fill", "none");
					segGrp.appendChild(ele);
				}

				if( panned ) {
					ele = document.createElementNS("http://www.w3.org/2000/svg", "rect");

					ele.setAttribute('x', pannedX - 50);
					ele.setAttribute('y', pannedY - 50);
					ele.setAttribute('width', 100);
					ele.setAttribute('height', 100);
					ele.setAttribute('stroke', 'yellow');
					ele.setAttribute('fill', 'none');
					ele.setAttribute('stroke-width', 4);
					ele.setAttribute('id', 'boundBox');

					segGrp.appendChild(ele);
				}
				// if the number of samples fixed is larger than 0,
				if( fixes['samples'].length > 0 ) {
					for( cell in fixes['samples'] ) {
						var bound = document.getElementById("N"+fixes['samples'][cell]['id']);

						if( bound != null ) {
								bound.setAttribute("fill", "yellow");
								bound.setAttribute("fill-opacity", "0.2");
						}
					}
				}
			}
  	});

}


//
//
//
//
function updateDataset() {

	curDataset = $('#dataset_sel').val();
	updateSlideList();
	updateClassifierList();
}



//
//	Update boundaries, if visible, to the appropriate colors based on
//	the selected classifier.
//
//
function updateClassifier() {

	var class_sel = document.getElementById('classifier_sel');


	curClassifier = class_sel.options[class_sel.selectedIndex].value;

 	//if( class_sel.selectedIndex != 0 ) {
	if( curClassifier != "none" ) {

		if( uid === null || classifierSession ) {
			// No active session, start a classification session
			//
			if( classifierSession == false ) {
				window.onbeforeunload = cleanupClassifierSession;
				classifierSession = true;
				$.ajax({
						url: "php/getSession.php",
						data: "",
						dataType: "json",
						success: function(data) {

							uid = data['uid'];
						}
				});
			}
			console.log("Load dataset "+curDataset+" and classifier "+curClassifier);

			$.ajax({
				type: "POST",
				url: "php/initClassifier.php",
				data: { dataset: curDataset,
						trainset: curClassifier },
				dataType: "json",
				success: function(data) {
					var	classNames = data['class_names'];

					box = " <svg width='20' height='20'> <rect width='15' height = '15' style='fill:lightgrey;stroke-width:3;stroke:rgb(0,0,0)'/></svg>";
					document.getElementById('negLegend').innerHTML = box + " " + classNames[0];
					box = " <svg width='20' height='20'> <rect width='15' height = '15' style='fill:lime;stroke-width:3;stroke:rgb(0,0,0)'/></svg>";
					document.getElementById('posLegend').innerHTML = box + " " + classNames[1];

					$('#retrainInfo').hide();
					$('#legend').show();
					$('#heatmap').hide();

					// Need to get the session UID
					$.ajax({
						url: "php/getSession.php",
						data: "",
						dataType: "json",
						success: function(data) {

							uid = data['uid'];
						}
					});

				}
			});
		} else {
			box = " <svg width='20' height='20'> <rect width='15' height = '15' style='fill:lightgrey;stroke-width:3;stroke:rgb(0,0,0)'/></svg>";
			document.getElementById('negLegend').innerHTML = box + " " + negClass;
			box = " <svg width='20' height='20'> <rect width='15' height = '15' style='fill:lime;stroke-width:3;stroke:rgb(0,0,0)'/></svg>";
			document.getElementById('posLegend').innerHTML = box + " " + posClass;

			$('#retrainInfo').show();
			$('#legend').show();
		}


	} else {

		$('#legend').hide();
	}

	if( overlayHidden === false ) {
		updateSeg();
	}
}




//
//	Display the appropriate heatmap (uncertain or positive class) when
//	a radio button is selected
//
function updateHeatmap() {

	if (application == "region"){
		$.ajax({
			type: "POST",
			url: "php/getHeatmap.php",
			data: { slide: curSlide,
							application: application,
							uid: uid },
			dataType: "json",
			success: function(data) {

			data = JSON.parse(data);

			annoGrp = document.getElementById('annoGrp');
			segGrp = document.getElementById('heatmapGrp');

			if( segGrp != null ) {
				segGrp.parentNode.removeChild(segGrp);
			}

			segGrp = document.createElementNS("http://www.w3.org/2000/svg", "g");
			segGrp.setAttribute('id', 'heatmapGrp');
			annoGrp.appendChild(segGrp);

			var xlinkns = "http://www.w3.org/1999/xlink";
			ele = document.createElementNS("http://www.w3.org/2000/svg", "image");
			ele.setAttributeNS(null, "x", 0);
			ele.setAttributeNS(null, "y", 0);
			ele.setAttributeNS(null, "width", data.width);
			ele.setAttributeNS(null, "height", data.height);
			ele.setAttributeNS(null, 'opacity', 0.25);
			ele.setAttribute('id', 'heatmapImg');

			uncertMin = data.uncertMin;
			uncertMax = data.uncertMax;
			classMin = data.classMin;
			classMax = data.classMax;

			if( $('#heatmapUncertain').is(':checked') ) {
				// heatmap should be reloaded with different time after updating heatmap image on local directory
				ele.setAttributeNS(xlinkns, "href", "heatmaps/"+uid+"/"+data.uncertFilename+"?v="+(new Date()).getTime());
				document.getElementById('heatMin').innerHTML = data.uncertMin.toFixed(2);
				document.getElementById('heatMax').innerHTML = data.uncertMax.toFixed(2);
			} else {
				ele.setAttributeNS(xlinkns, "href", "heatmaps/"+uid+"/"+data.classFilename+"?v="+(new Date()).getTime());
				document.getElementById('heatMin').innerHTML = data.classMin.toFixed(2);
				document.getElementById('heatMax').innerHTML = data.classMax.toFixed(2);
			}
			segGrp.appendChild(ele);

			heatmapLoaded = true;
			console.log("Uncertainty min: "+uncertMin+", max: "+uncertMax+", median: "+data.uncertMedian);
			}
		});
	} else{
		heatmapLoaded = false;
		updateSeg();

	}

}




//
//	Update annotation and viewport information when the view changes
//  due to panning or zooming.
//
//
function onImageViewChanged(event) {
	var boundsRect = viewer.viewport.getBounds(true);

	// Update viewport information. dataportXXX is the view port coordinates
	// using pixel locations. ie. if dataPortLeft is  0 the left edge of the
	// image is aligned with the left edge of the viewport.
	//
	statusObj.viewportX(boundsRect.x);
	statusObj.viewportY(boundsRect.y);
	statusObj.viewportW(boundsRect.width);
	statusObj.viewportH(boundsRect.height);
	statusObj.dataportLeft(imgHelper.physicalToDataX(imgHelper.logicalToPhysicalX(boundsRect.x)));
	statusObj.dataportTop(imgHelper.physicalToDataY(imgHelper.logicalToPhysicalY(boundsRect.y)) * imgHelper.imgAspectRatio);
	statusObj.dataportRight(imgHelper.physicalToDataX(imgHelper.logicalToPhysicalX(boundsRect.x + boundsRect.width)));
	statusObj.dataportBottom(imgHelper.physicalToDataY(imgHelper.logicalToPhysicalY(boundsRect.y + boundsRect.height))* imgHelper.imgAspectRatio);
	statusObj.scaleFactor(imgHelper.getZoomFactor());

	var p = imgHelper.logicalToPhysicalPoint(new OpenSeadragon.Point(0, 0));

	svgOverlayVM.annoGrpTranslateX(p.x);
	svgOverlayVM.annoGrpTranslateY(p.y);
	svgOverlayVM.annoGrpScale(statusObj.scaleFactor());

	var annoGrp = document.getElementById('annoGrp');
	annoGrp.setAttribute("transform", annoGrpTransformFunc());
}






//
//	Retreive the boundaries for nuclei within the viewport bounds and an
//	area surrounding the viewport. The are surrounding the viewport is a
//	border the width and height of the viewport. This allows the user to pan a full
//	viewport width or height before having to fetch new boundaries.
//
//
function updateSeg() {

	var ele, segGrp, annoGrp;

	if( statusObj.scaleFactor() > 0.5 ) {

		var left, right, top, bottom, width, height;

		// Grab nuclei a viewport width surrounding the current viewport
		//
		width = statusObj.dataportRight() - statusObj.dataportLeft();
		height = statusObj.dataportBottom() - statusObj.dataportTop();

		left = (statusObj.dataportLeft() - width > 0) ?	statusObj.dataportLeft() - width : 0;
		right = statusObj.dataportRight() + width;
		top = (statusObj.dataportTop() - height > 0) ?	statusObj.dataportTop() - height : 0;
		bottom = statusObj.dataportBottom() + height;

		var class_sel = document.getElementById('classifier_sel');

	    $.ajax({
			type: "POST",
       	 	url: "db/getnuclei.php",
       	 	dataType: "json",
			data: { uid:	uid,
					slide: 	curSlide,
					left:	left,
					right:	right,
					top:	top,
					bottom:	bottom,
					dataset: curDataset,
					trainset: curClassifier,
					application: application
			},

			success: function(data) {

					segGrp = document.getElementById('segGrp');
					annoGrp = document.getElementById('anno');

					// Save current viewport location
					boundsLeft = statusObj.dataportLeft();
					boundsRight = statusObj.dataportRight();
					boundsTop = statusObj.dataportTop();
					boundsBottom = statusObj.dataportBottom();

					// If group exists, delete it
					if( segGrp != null ) {
						segGrp.parentNode.removeChild(segGrp);
					}

					// Create segment group
          segGrp = document.createElementNS("http://www.w3.org/2000/svg", "g");
          segGrp.setAttribute('id', 'segGrp');
          annoGrp.appendChild(segGrp);


					if (application == "cell"){
						for( cell in data ) {
							ele = document.createElementNS("http://www.w3.org/2000/svg", "circle");

							ele.setAttribute('cx', data[cell][1]);
							ele.setAttribute('cy', data[cell][2]);
							ele.setAttribute('r', 2);
							ele.setAttribute('id', 'N' + data[cell][0]);
							ele.setAttribute('stroke', data[cell][3]);
							ele.setAttribute('fill',  data[cell][3]);

							segGrp.appendChild(ele);
						}
					} else{
						for( cell in data ) {
							ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");

							ele.setAttribute('points', data[cell][0]);
							ele.setAttribute('id', 'N' + data[cell][1]);
							if (application == "region"){
								ele.setAttribute('stroke', 'aqua');
								ele.setAttribute('fill', data[cell][2]);
								ele.setAttribute("fill-opacity", "0.2");
								ele.setAttribute('stroke-width', 4);
								ele.setAttribute("stroke-dasharray", "5,5");
							} else{
								ele.setAttribute('stroke', data[cell][2]);
								ele.setAttribute('fill', 'none');
							}
							segGrp.appendChild(ele);
						}
					}


					if( panned ) {
						ele = document.createElementNS("http://www.w3.org/2000/svg", "rect");

						ele.setAttribute('x', pannedX - 50);
						ele.setAttribute('y', pannedY - 50);
						ele.setAttribute('width', 100);
						ele.setAttribute('height', 100);
						ele.setAttribute('stroke', 'yellow');
						ele.setAttribute('fill', 'none');
						ele.setAttribute('stroke-width', 4);
						ele.setAttribute('id', 'boundBox');

						segGrp.appendChild(ele);
					}
					// if the number of samples fixed is larger than 0,
					if( fixes['samples'].length > 0 ) {
						for( cell in fixes['samples'] ) {
							var bound = document.getElementById("N"+fixes['samples'][cell]['id']);

							if( bound != null ) {
								if (application == "region"){
									bound.setAttribute("fill", "yellow");
									bound.setAttribute("fill-opacity", "0.2");
								} else{
									bound.setAttribute('stroke', 'yellow');
								}
							}
						}
					}
				}
    	});

			// set heatmapLoaded to false after retraining
			heatmapLoaded = false;

	} else {

		// Only display heatmap for active sessions
		//
		if( curSlide != "" && curClassifier != 'none' && classifierSession == false && heatmapLoaded == false ) {

		    $.ajax({
				type: "POST",
    	   	 	url: "php/getHeatmap.php",
    	   	 	dataType: "json",
				data: { uid:	uid,
						slide: 	curSlide,
						application: 	application,
				},

				success: function(data) {

					data = JSON.parse(data);

					annoGrp = document.getElementById('annoGrp');
					segGrp = document.getElementById('heatmapGrp');

					if( segGrp != null ) {
						segGrp.parentNode.removeChild(segGrp);
					}

					segGrp = document.createElementNS("http://www.w3.org/2000/svg", "g");
					segGrp.setAttribute('id', 'heatmapGrp');
					annoGrp.appendChild(segGrp);

					var xlinkns = "http://www.w3.org/1999/xlink";
					ele = document.createElementNS("http://www.w3.org/2000/svg", "image");
					ele.setAttributeNS(null, "x", 0);
					ele.setAttributeNS(null, "y", 0);
					ele.setAttributeNS(null, "width", data.width);
					ele.setAttributeNS(null, "height", data.height);
					ele.setAttributeNS(null, 'opacity', 0.25);
					ele.setAttribute('id', 'heatmapImg');

					uncertMin = data.uncertMin;
					uncertMax = data.uncertMax;
					classMin = data.classMin;
					classMax = data.classMax;

					if( $('#heatmapUncertain').is(':checked') ) {
						// heatmap should be reloaded with different time after updating heatmap image on local directory
						ele.setAttributeNS(xlinkns, "href", "heatmaps/"+uid+"/"+data.uncertFilename+"?v="+(new Date()).getTime());
						document.getElementById('heatMin').innerHTML = data.uncertMin.toFixed(2);
						document.getElementById('heatMax').innerHTML = data.uncertMax.toFixed(2);
					} else {
						ele.setAttributeNS(xlinkns, "href", "heatmaps/"+uid+"/"+data.classFilename+"?v="+(new Date()).getTime());
						document.getElementById('heatMin').innerHTML = data.classMin.toFixed(2);
						document.getElementById('heatMax').innerHTML = data.classMax.toFixed(2);
					}
					segGrp.appendChild(ele);

					heatmapLoaded = true;
					console.log("Uncertainty min: "+uncertMin+", max: "+uncertMax+", median: "+data.uncertMedian);
				}
			});
		}
	}
}

//
// Update colors when a sample is selected
// Parameters
// selectedJSON id
//
function updateBoundColors(obj) {

	for( cell in fixes['samples'] ) {
		var bound = document.getElementById("N"+fixes['samples'][cell]['id']);

		if( bound != null ) {
			if (fixes['samples'][cell]['id'] == obj['id']){
				if (application == "region"){
					bound.setAttribute('fill', 'yellow');
					bound.setAttribute("fill-opacity", "0.2");
				} else if (application == "cell"){
					bound.setAttribute('stroke', 'yellow');
					bound.setAttribute('fill', 'yellow');
				} else{
				bound.setAttribute('stroke', 'yellow');
				}
			}
		}
	}
}

//
// Undo colors when a sample is deleted
// Parameters
// selectedJSON id
//
function undoBoundColors(obj) {

	for( cell in fixes['samples'] ) {
		var bound = document.getElementById("N"+fixes['samples'][cell]['id']);

		if( bound != null ) {
			// check id
			if (fixes['samples'][cell]['id'] == obj['id']){
				// check label
				if( fixes['samples'][cell]['label'] == -1 ) {
					if (application == "region"){
						bound.setAttribute('fill', 'lime');
						bound.setAttribute("fill-opacity", "0.2");

					} else if(application == "cell"){
						bound.setAttribute('stroke', 'lime');
						bound.setAttribute('fill', 'lime');

					}
					else{
						bound.setAttribute('stroke', 'lime');
					}

				} else if( fixes['samples'][cell]['label'] == 1 ) {
					if (application == "region"){
						bound.setAttribute('fill', 'lightgrey');
						bound.setAttribute("fill-opacity", "0.2");

					} else if (application == "cell"){
						bound.setAttribute('stroke', 'lightgrey');
						bound.setAttribute('fill', 'lightgrey');

					}else{
						bound.setAttribute('stroke', 'lightgrey');
					}

				}
			}
		}
	}
}


function nucleiSelect() {

	if( classifierSession == false ) {
		$.ajax({
		    type:   "POST",
		    url:    "db/getsingle.php",
		    dataType: "json",
		    data:   { slide:    curSlide,
		              cellX:    Math.round(statusObj.mouseImgX()),
		              cellY:    Math.round(statusObj.mouseImgY()),
									application: application,
		            },
		    success: function(data) {
		            if( data !== null ) {

						if( curClassifier === "none" ) {
							// No classifier applied, just log results
			                console.log(curSlide+","+data[2]+","+data[3]+", id: "+data[1]);
            }
						else {
	          	// We're adding an object, make sure the retrain button is enabled.
	          	$('#retrainBtn').removeAttr('disabled');

							var	obj = {slide: curSlide, centX: data[2], centY: data[3], label: 0, id: data[1]};
	          	var cell = document.getElementById("N"+obj['id']);
							// initializes a flag to check the status of undo
							var undo = false;

							// Flip the label here. lime indicates the positive class, so we
							// want to change the label to -1. Change to 1 for lightgrey. If
							// the color is niether, the sample has been picked already so
							// ignore.
							//
							if (application == "region"){
								if( cell.getAttribute('fill') === "lime" ) {
									obj['label'] = -1;
								} else if( cell.getAttribute('fill') === "lightgrey" ) {
									obj['label'] = 1;
								// if the color of the selected cell is yellow
								}	else if( cell.getAttribute('fill') === "yellow" ) {
										// find a cell with the same id
										for( cell in fixes['samples'] ) {
											if (fixes['samples'][cell]['id'] == obj['id']){
												obj['label'] = fixes['samples'][cell]['label'];
											}
										}
										undo = true;
								}
							}else {
								if( cell.getAttribute('stroke') === "lime" ) {
									obj['label'] = -1;
								} else if( cell.getAttribute('stroke') === "lightgrey" ) {
									obj['label'] = 1;
								// if the color of the selected cell is yellow
								}	else if( cell.getAttribute('stroke') === "yellow" ) {
										// find a cell with the same id
										for( cell in fixes['samples'] ) {
											if (fixes['samples'][cell]['id'] == obj['id']){
												obj['label'] = fixes['samples'][cell]['label'];
											}
										}
										undo = true;
								}

							}

							// if the cell is already selected
							if (undo){
								// call undoBoundColrs to undo the color
								undoBoundColors(obj);
								for( cell in fixes['samples'] ) {
									if (fixes['samples'][cell]['id'] == obj['id']){
										fixes['samples'].splice(cell, 1);
									}
								}
								statusObj.samplesToFix(statusObj.samplesToFix()-1);
								undo = false;
							}
							// else if the cell is labeled to -1 or 1
							else if( (obj['label'] == -1) || (obj['label'] == 1) ) {
                fixes['samples'].push(obj);
								// call updateBoundColors to update to color
								updateBoundColors(obj);
                statusObj.samplesToFix(statusObj.samplesToFix()+1);
          		}
          	}
          }
        }
		});
	}
}





function retrain() {

	// Set iteration to -1 to indicate these are hand-picked
	fixes['iteration'] = -1;

	// Display the progress dialog...
	$('#progDiag').modal('show');

	var delay = 2000;


	$.ajax({
		type: "POST",
		url: "php/submitSamples.php",
		dataType: "json",
		data: fixes,
		success: function(result) {

			console.log(result);

			// Clear submitted samples
			fixes['samples'] = [];
			statusObj.samplesToFix(0);

			// Update classification results.
			updateSeg();
			if (application == "region"){
				updateHeatmap();
			}

			setTimeout(function() {
				// Hide progress dialog
				$('#progDiag').modal('hide');
		        }, delay);

		}
	});
}





//
// ===============	Mouse event handlers for viewer =================
//

//
//	Mouse enter event handler for viewer
//
//
function onMouseEnter(event) {
	statusObj.haveMouse(true);
}


//
// Mouse move event handler for viewer
//
//
function onMouseMove(event) {
	var offset = osdCanvas.offset();

	statusObj.mouseRelX(event.pageX - offset.left);
	statusObj.mouseRelY(event.pageY - offset.top);
	statusObj.mouseImgX(imgHelper.physicalToDataX(statusObj.mouseRelX()));
	statusObj.mouseImgY(imgHelper.physicalToDataY(statusObj.mouseRelY()));
}


//
//	Mouse leave event handler for viewer
//
//
function onMouseLeave(event) {
	statusObj.haveMouse(false);
}





function onMouseClick(event) {
		if (application == "region"){
			event.preventDefaultAction = true;

		}
    clickCount++;
    if( clickCount === 1 ) {
        // If no click within 250ms, treat it as a single click
        singleClickTimer = setTimeout(function() {
                    // Single click
                    clickCount = 0;
                }, 250);
    } else if( clickCount >= 2 ) {
        // Double click
        clearTimeout(singleClickTimer);
				if( segDisplayOn ) {
        clickCount = 0;
        nucleiSelect();
			}
    }
}



//
// =======================  Button Handlers ===================================
//



//
//	Load the boundaries for the current slide and display
//
//
function viewSegmentation() {

	var	segBtn = $('#btn_1');

	if( segDisplayOn ) {
		// Currently displaying segmentation, hide it
		segBtn.val("Show Segmentation");
		$('.overlaySvg').css('visibility', 'hidden');
		segDisplayOn = false;
	} else {
		// Segmentation not currently displayed, show it
		segBtn.val("Hide Segmentation");
		$('.overlaySvg').css('visibility', 'visible');
		segDisplayOn = true;

		updateSeg();
		if (application == "region"){
			updateHeatmap();
		}
	}
}




function go() {

	var	segBtn = $('#btn_1');

	pannedX = $("#x_pos").val();
	pannedY = $("#y_pos").val();

	// TODO! - Need to validate location against size of image
	if( pannedX === "" || pannedY === "" ) {
		window.alert("Invalid position");
	} else {

		// Turn on overlay and reset bounds to force update
		segBtn.val("Hide Segmentation");
		$('.overlaySvg').css('visibility', 'visible');
		segDisplayOn = true;
		boundsLeft = boundsRight = boundsTop = boundsBottom = 0;

		// Zoom in all the way
		viewer.viewport.zoomTo(viewer.viewport.getMaxZoom());

		// Move to nucei
		imgHelper.centerAboutLogicalPoint(new OpenSeadragon.Point(imgHelper.dataToLogicalX(pannedX),
															  imgHelper.dataToLogicalY(pannedY)));
		panned = true;
	}
}







//
// Retruns the value of the GET request variable specified by name
//
//
function $_GET(name) {
	var match = RegExp('[?&]' + name + '=([^&]*)').exec(window.location.search);
	return match && decodeURIComponent(match[1].replace(/\+/g,' '));
}







//
// Image data we want knockout.js to keep track of
//
var statusObj = {
	haveImage: ko.observable(false),
	haveMouse: ko.observable(false),
	imgAspectRatio: ko.observable(0),
	imgWidth: ko.observable(0),
	imgHeight: ko.observable(0),
	mouseRelX: ko.observable(0),
	mouseRelY: ko.observable(0),
	mouseImgX: ko.observable(0),
	mouseImgY: ko.observable(0),
	scaleFactor: ko.observable(0),
	viewportX: ko.observable(0),
	viewportY: ko.observable(0),
	viewportW: ko.observable(0),
	viewportH: ko.observable(0),
	dataportLeft: ko.observable(0),
	dataportTop: ko.observable(0),
	dataportRight: ko.observable(0),
	dataportBottom: ko.observable(0),
	samplesToFix: ko.observable(0)
};


var svgOverlayVM = {
	annoGrpTranslateX:	ko.observable(0.0),
	annoGrpTranslateY:	ko.observable(0.0),
	annoGrpScale: 		ko.observable(1.0),
	annoGrpTransform:	annoGrpTransformFunc
};

var vm = {
	statusObj:	ko.observable(statusObj),
	svgOverlayVM: ko.observable(svgOverlayVM)
};



// Apply binfding for knockout.js - Let it keep track of the image info
// and mouse positions
//
ko.applyBindings(vm);
