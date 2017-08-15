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
var uid = "";
var classifier = "";
var negClass = "";
var posClass = "";
var IIPServer = "";
var SlideSuffix = ".svs-tile.dzi.tif";
var SlideLocPre = "&RGN=";
var SlideLocSuffix = "&CVT=jpeg";

var viewer = null, imgHelper = null, osdCanvas = null;
var olDiv = null;

var lastScaleFactor = 0;
var	sampleDataJson = "";
var	boxes = ["box_1", "box_2", "box_3", "box_4", "box_5", "box_6","box_7", "box_8"];
var curDataset;
var curBox = -1;
var curX = 0, curY = 0;

var boundaryOn = true;
var reloaded = false;
var application = "";

//
//	Initialization
//
//
$(function() {

	application = $_GET("application");


	document.getElementById("home").setAttribute("href","index_home.html?application="+application);
	document.getElementById("nav_review").setAttribute("href","review.html?application="+application);
	document.getElementById("viewer").setAttribute("href","viewer.html?application="+application);
	document.getElementById("nav_heatmaps").setAttribute("href","heatmaps.html?application="+application);

	// Setup the thumbnail scroller
	//
	var	width = 0;


	$('#overflow .slider div').each(function() {
		width += $(this).outerWidth(true);
	});

	$('#overflow .slider').css('width', width + "px");

	// get session vars
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
			reloaded = data['reloaded'];

			if( reloaded == true ) {
				statusObj.iteration(data['iteration'])
			}

			if( uid == null ) {
				window.alert("No session active");
				window.history.back();
			}
		}
	});

	if (statusObj.iteration() === 0){
		// Display the progress dialog...
		$('#progDiag').modal('show');
	}

	// Create the slide zoomer, update slide count etc...
	// We will load the tile pyramid after the slide list is loaded
	//
	viewer = new OpenSeadragon.Viewer({ showNavigator: true, id: "slideZoom", prefixUrl: "images/", animationTime: 0.1});
	imgHelper = viewer.activateImagingHelper({onImageViewChanged: onImageViewChanged});

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

		// Zoom and pan to selected nuclei
		homeToNuclei();
	});



	viewer.addHandler('close', function(event) {
		osdCanvas = $(viewer.canvas);
		statusObj.haveImage(false);

		//viewer.drawer.clearOverlays();
        osdCanvas.off('mouseenter.osdimaginghelper', onMouseEnter);
        osdCanvas.off('mousemove.osdimaginghelper', onMouseMove);
        osdCanvas.off('mouseleave.osdimaginghelper', onMouseLeave);

		osdCanvas = null;
		statusObj.curSlide("");

	});


	viewer.addHandler('animation-finish', function(event) {

		var annoGrp = document.getElementById('annoGrp');
		var sampGrp = document.getElementById('sample');

		if( sampGrp != null ) {
			sampGrp.parentNode.removeChild(sampGrp);
		}

		if( annoGrp != null ) {
			sampGrp = document.createElementNS("http://www.w3.org/2000/svg", "g");
			sampGrp.setAttribute('id', 'sample');
			annoGrp.appendChild(sampGrp);

			if (application == "region"){
				ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
				ele.setAttribute('points', sampleDataJson['samples'][curBox]['boundary']);
				ele.setAttribute('id', 'boundary');
				ele.setAttribute('fill', 'yellow');
				ele.setAttribute("fill-opacity", "0.2");
			}	else {
				ele = document.createElementNS("http://www.w3.org/2000/svg", "rect");
				ele.setAttribute('x', curX - 50);
				ele.setAttribute('y', curY - 50);
				ele.setAttribute('width', 100);
				ele.setAttribute('height', 100);
				ele.setAttribute('stroke', 'yellow');
				ele.setAttribute('fill', 'none');
				ele.setAttribute('stroke-width', 4);
				ele.setAttribute('id', 'boundBox');
				sampGrp.appendChild(ele);

				if (application == "nuclei"){
					ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
					ele.setAttribute('points', sampleDataJson['samples'][curBox]['boundary']);
					ele.setAttribute('id', 'boundary');
					ele.setAttribute('stroke', 'yellow');
					ele.setAttribute('fill', 'none');
				} else{
					ele = document.createElementNS("http://www.w3.org/2000/svg", "circle");
					ele.setAttribute('cx', sampleDataJson['samples'][curBox]['centX']);
					ele.setAttribute('cy', sampleDataJson['samples'][curBox]['centY']);
					ele.setAttribute('r', 2);
					ele.setAttribute('id', 'boundary');
					ele.setAttribute('stroke', 'yellow');
					ele.setAttribute('fill', 'yellow');
				}
			}



			if( boundaryOn ) {
				ele.setAttribute('visibility', 'visible');
				// Make sure toggle button refects the correct action and is enabled
				$('#toggleBtn').val("Hide segmentation");
				$('#toggleBtn').removeAttr('disabled');
			} else {
				ele.setAttribute('visibility', 'hidden');
			}
			sampGrp.appendChild(ele);

			$('.overlaySvg').css('visibility', 'visible');
		}
	});



	// Assign click handlers to each of the thumbnail divs
	//
	boxes.forEach(function(entry) {

		var	box = document.getElementById(entry);
		var	clickCount = 0;

		box.addEventListener('click', function() {
			clickCount++;
			if( clickCount === 1 ) {
				singleClickTimer = setTimeout(function() {
					clickCount = 0;
					thumbSingleClick(entry);
				}, 200);
			} else if( clickCount === 2 ) {
				clearTimeout(singleClickTimer);
				clickCount = 0;
				thumbDoubleClick(entry);
			}
		}, false);
	});


	// Update the thumbnails
	updateSamples();

});






//
//	A double click in the thumbnail box toggles the current classification
//	of the object.
//
//
function thumbDoubleClick(box) {

	var index = boxes.indexOf(box);
	var label = sampleDataJson['samples'][index]['label'];

	// Toggle through the 3 states, pos, neg and ignore
	//
	if( label === 1 ) {
		sampleDataJson['samples'][index]['label'] = -1;
	} else if( label === -1 ) {
		sampleDataJson['samples'][index]['label'] = 0;
	} else {
		sampleDataJson['samples'][index]['label'] = 1;
	}
	updateClassStatus(index);
};






//
// A single click in the thumbnail box loads the appropriate slide into the viewer
// and pans and zooms to the specific object.
//
//
function thumbSingleClick(box) {

	// Load the appropriate slide in the viewer
	var index = boxes.indexOf(box);
	if( curBox != index ) {

		var newSlide = sampleDataJson['samples'][index]['slide'];

		// Slide loading process pans to the current nuclei, make sure
		// curX and curY are updated before loading a new slide.
		//
		curX = Math.round(sampleDataJson['samples'][index]['centX']);
		curY = Math.round(sampleDataJson['samples'][index]['centY']);

		if( statusObj.curSlide() == "" ) {
			statusObj.curSlide(newSlide);
			updateSlideView();
 		} else {
 			if( statusObj.curSlide() != newSlide ) {
				viewer.close();
				statusObj.curSlide(newSlide);
				updateSlideView();
			} else {
				// On same slide,, no need to load it again
				homeToNuclei();
 			}
		}

		// Mark the selected box with a gray background
		var boxDiv = "#"+box;
		$(boxDiv).css('background', '#CCCCCC');

		// Clear previously selected box if there was one
		if( curBox != -1 && curBox != index ) {
			boxDiv = "#"+boxes[curBox];
			$(boxDiv).css('background', '#FFFFFF');
		}
		curBox = index;
		boundaryOn = true;
	}
};






function updateSlideView() {


	$.ajax({
		type: "POST",
		url: "db/getPyramidPath.php",
		dataType: "json",
		data: { slide: statusObj.curSlide() },
		success: function(data) {

				// Zoomer needs '.dzi' appended to the end of the filename
				pyramid = "DeepZoom="+data[0]+".dzi";
				viewer.open(IIPServer + pyramid);
		}
	});
}





function homeToNuclei() {

	// Zoom in all the way
	viewer.viewport.zoomTo(viewer.viewport.getMaxZoom());
	// Move to nucei
	imgHelper.centerAboutLogicalPoint(new OpenSeadragon.Point(imgHelper.dataToLogicalX(curX),
															  imgHelper.dataToLogicalY(curY)));
}






function updateSamples() {

	$.ajax({
		type: "POST",
		url: "php/selectSamples.php",
		data: {application, application},
		dataType: "json",
		success: function(data) {

			sampleDataJson = data;

			// Clear the slide viewer if there's something showing
			//
			if( statusObj.curSlide() != "" ) {

				viewer.close();
				statusObj.curSlide("");
			};

			var slide, centX, centY, sizeX, sizeY, loc, thumbNail, scale;
			var sampleArray = data['samples'];
			var scale_cent = 25;
			var scale_size = 50.0;

			if (application == "region"){
				scale_cent = 64;
				scale_size = 128.0;
			}
			statusObj.iteration(data['iteration']);
			statusObj.accuracy(data['accuracy']);

			for( sample in sampleArray ) {

				thumbTag = "#thumb_"+(parseInt(sample)+1);
				labelTag = "#label_"+(parseInt(sample)+1);
				boxTag = "#"+boxes[sample];
				scale = sampleArray[sample]['scale'];
				slide = sampleArray[sample]['slide'];

				centX = (sampleArray[sample]['centX'] - (scale_cent * scale)) / sampleArray[sample]['maxX'];
				centY = (sampleArray[sample]['centY'] - (scale_cent * scale)) / sampleArray[sample]['maxY'];
				sizeX = (scale_size * scale) / sampleArray[sample]['maxX'];
				sizeY = (scale_size * scale) / sampleArray[sample]['maxY'];
				loc = centX+","+centY+","+sizeX+","+sizeY;

				thumbNail = IIPServer+"FIF="+sampleArray[sample]['path']+SlideLocPre+loc+"&WID=100"+SlideLocSuffix;

				$(thumbTag).attr("src", thumbNail);
				updateClassStatus(sample);

				// Hide progress dialog
				$('#progDiag').modal('hide');

				// Make sure overlay is hidden
				$('.overlaySvg').css('visibility', 'hidden');

				// Disable button
				$('#toggleBtn').attr('disabled', 'disabled');

				// Clear grid selection
				if( curBox != -1 ) {
					boxDiv = "#"+boxes[curBox];
					$(boxDiv).css('background', '#FFFFFF');
					curBox = -1;
				}
			}
			// Select first sample automatically
			thumbSingleClick("box_1");

		},
		error: function() {
			console.log("Selection failed");
		}
	});
}



function updateClassStatus(sample) {

	var labelTag = "#label_"+(parseInt(sample)+1),
		label = $('#box_'+(parseInt(sample)+1)).children(".classLabel")

	label.removeClass("negLabel");
	label.removeClass("ignoreLabel");
	label.removeClass("posLabel");

	if( sampleDataJson['samples'][sample]['label'] === 1 ) {
		$(labelTag).text(posClass);
		label.addClass("posLabel");
	} else if( sampleDataJson['samples'][sample]['label'] === -1 ) {
		$(labelTag).text(negClass);
		label.addClass("negLabel");
	} else {
		$(labelTag).text("Ignore");
		label.addClass("ignoreLabel");
	}
}



// --------  Buton handlers ---------------------------------------------------


function cancelSession() {
	$.ajax({
		url: "php/cancelSession.php",
		data: "",
		success: function() {
			window.location = "index_home.html?application="+application;
		}
	});
}




function submitLabels() {

	var itemTag;

	// Display the progress dialog...
	$('#progDiag').modal('show');

	// No need to send boundaries to the server
	for( i = 0; i < sampleDataJson['samples'].length; i++ ) {
		sampleDataJson['samples'][i]['boundary'] = "";
	}

	$.ajax({
		type: "POST",
		url: "php/submitSamples.php",
		dataType: "json",
		data: sampleDataJson,
		success: function() {

			// Get a new set of samples
			updateSamples();
		}
	});
}





function saveSession() {

	$('#saveDiag').modal('show');

	if( reloaded ) {
		$.ajax({
			url: "php/finishReloadedSession.php",
			data: "",
			success: function(data) {

				window.location = "index_home.html?application="+application;
			},
		});

	} else {
		$.ajax({
			url: "php/finishSession.php",
			data: "",
			success: function(data) {

				window.location = "index_home.html?application="+application;
			},
		});
	}
}







function toggleSegVisibility() {

	var boundary = document.getElementById('boundary');
	if( boundary != null ) {

		if( boundaryOn ) {
			$('#toggleBtn').val("Show segmentation");
			boundary.setAttribute('visibility', 'hidden');
			boundaryOn = false;
		} else {
			$('#toggleBtn').val("Hide segmentation");
			boundary.setAttribute('visibility', 'visible');
			boundaryOn = true;
		}
	}
}




//-----------------------------------------------------------------------------


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






function updateOverlayInfo() {

	// Only update the scale of the svg if it has changed. This speeds up
	// scrolling through the image.
	//
	if( lastScaleFactor != statusObj.scaleFactor() ) {
		lastScaleFactor = statusObj.scaleFactor();
		var annoGrp = document.getElementById('anno');

		if( annoGrp != null ) {
			var scale = "scale(" + statusObj.scaleFactor() + ")";
			annoGrp.setAttribute("transform", scale);
		}
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

	statusObj.mouseX(imgHelper.dataToLogicalX(offset.left));
	statusObj.mouseY(imgHelper.dataToLogicalX(offset.top));
	statusObj.mouseRelX(event.pageX - offset.left);
	statusObj.mouseRelY(event.pageY - offset.top);
	statusObj.mouseImgX(imgHelper.physicalToDataX(statusObj.mouseRelX()));
	statusObj.mouseImgY(imgHelper.physicalToDataY(statusObj.mouseRelY()));
	statusObj.mouseLogX(imgHelper.dataToLogicalX(statusObj.mouseImgX()));
	statusObj.mouseLogY(imgHelper.dataToLogicalY(statusObj.mouseImgY()));
}





//
//	Mouse leave event handler for viewer
//
//
function onMouseLeave(event) {
	statusObj.haveMouse(false);
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
	mouseLogX: ko.observable(0),
	mouseLogY: ko.observable(0),
	mouseX: ko.observable(0),
	mouseY: ko.observable(0),
	scaleFactor: ko.observable(0),
	viewportX: ko.observable(0),
	viewportY: ko.observable(0),
	viewportW: ko.observable(0),
	viewportH: ko.observable(0),
	dataportLeft: ko.observable(0),
	dataportTop: ko.observable(0),
	dataportRight: ko.observable(0),
	dataportBottom: ko.observable(0),
	iteration:	ko.observable(0),
	accuracy:	ko.observable(0.0),
	imgReady: ko.observable(false),
	curSlide: ko.observable("")
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
