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
var IIPServer = "";



var SlideSuffix = ".svs-tile.dzi.tif";
var SlideLocPre = "&RGN=";
var SlideLocSuffix = "&CVT=jpeg";

var uid = "";
var testset = "";
var negClass = "";
var posClass = "";
var viewer = null;
var imgHelper = null, osdCanvas = null, viewerHook = null;
var curSlide = "", curDataset = "";

var displaySeg = false, selectNuc = false;
var lastScaleFactor = 0;
var clickCount = 0;

var	selectedJSON = [];
var pyramids;
var	boxes = ["box_1", "box_2", "box_3", "box_4", "box_5", "box_6","box_7", "box_8"];

var boundsLeft = 0, boundsRight = 0, boundsTop = 0, boundsBottom = 0;
var defaultClass;

var reloaded = false;
var application = "";
var scale = 0.5;

//
//	Initialization
//
//
$(function() {

	application = $_GET("application");

	document.getElementById("revBtn").setAttribute("onClick", "window.location='picker_review.html?application="+application+"'");

	// Setup the grid slider relative to the window width
	width = 0;
	$('#overflow .slider div').each(function() {
		width += $(this).outerWidth(true);
	});
	$('#overflow .slider').css('width', width + "px");


	// Create the slide zoomer, add event handlers, etc...
	// We will load the tile pyramid after the slide list is loaded
	//
	viewer = new OpenSeadragon.Viewer({ showNavigator: true, id: "slideZoom", prefixUrl: "images/", animationTime: 0.1});
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

	});


	viewer.addHandler('close', function(event) {
		osdCanvas = $(viewer.canvas);
		statusObj.haveImage(false);

        osdCanvas.off('mouseenter.osdimaginghelper', onMouseEnter);
        osdCanvas.off('mousemove.osdimaginghelper', onMouseMove);
		osdCanvas.off('mouseleave.osdimaginghelper', onMouseLeave);

		osdCanvas = null;
	});


	viewer.addHandler('animation-finish', function(event) {

		if( displaySeg ) {

			if (application == "region") {
				scale = 0.2;
			}

			if( statusObj.scaleFactor() > scale ) {
				$('.overlaySvg').css('visibility', 'visible');
				var centerX = statusObj.dataportLeft() +
							  ((statusObj.dataportRight() - statusObj.dataportLeft()) / 2);
				var centerY = statusObj.dataportTop() +
							  ((statusObj.dataportBottom() - statusObj.dataportTop()) / 2);

				if( centerX < boundsLeft || centerX > boundsRight ||
					centerY < boundsTop || centerY > boundsBottom ) {
					console.log("Updating segmentation");
					updateSeg();
				}
			} else {
				$('.overlaySvg').css('visibility', 'hidden');
			}
		}
	});



	// get session vars and load the first slide
	$.ajax({
		url: "php/getSession.php",
		data: "",
		dataType: "json",
		success: function(data) {

			uid = data['uid'];
			// Reusing getSession.php for picker, className is the testset name
			testset = data['className'];
			posClass = data['posClass'];
			negClass = data['negClass'];
			curDataset = data['dataset'];
			IIPServer = data['IIPServer'];
			reloaded = data['reloaded'];


			if( uid == null ) {
				window.alert("No session active");
				window.history.back();
			} else {
				updateSlideList();
				$('#posLabel').text(posClass);
				$('#negLabel').text(negClass);
				document.getElementById('radioNeg').innerHTML = negClass;
				document.getElementById('radioPos').innerHTML = posClass;

				defaultClass = -1;
			}
		}
	});

	// Get number of objects added to test set already
	//
	$.ajax({
		type: "POST",
		url: "php/getObjCount.php",
		dataType: "json",
		success: function(data) {
			if( data['status'] === "PASS" ) {
				statusObj.testSetCnt(data['count']);
				if( data['count'] > 0 ) {
					$('#saveBtn').removeAttr('disabled');
					$('#revBtn').removeAttr('disabled');
				}
			} else {
				// TODO - Indicate failure
				console.log("Getting object count failed");
			}
		}
	});

	// Set the update handler for the slide selector
	$("#slideSel").change(updateSlide);
	clickCount = 0;

	// Handler for default class radio buttons
	$('input[name=classSel]').change(updateDefaultClass);

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
});






function updateSlideList() {
	var slideSel = $("#slideSel");

	// Get the list of slides for the current dataset
	$.ajax({
		type: "POST",
		url: "db/getslides.php",
		data: { dataset: curDataset },
		dataType: "json",
		success: function(data) {

			pyramids = data['paths'];
			curSlide = String(data['slides'][0]);		// Start with the first slide in the list

			slideSel.empty();
			// Add the slides we have segmentation boundaries for to the dropdown
			// selector
			for( var item in data['slides'] ) {
				slideSel.append(new Option(data['slides'][item], data['slides'][item]));
			}

			// Get the slide pyrimaid and display
			updateSlideView();
		}
	});
}







function updateSlideView() {

	// Zoomer needs '.dzi' appended to the end of the file
	pyramid = "DeepZoom="+pyramids[$('#slideSel').prop('selectedIndex')]+".dzi";
	viewer.open(IIPServer + pyramid);
}





//
//	A new slide has been selected from the drop-down menu, update the
// 	slide zoomer.
//
//
function updateSlide() {

	curSlide = $('#slideSel').val();
	updateSlideView();
}





function updateDefaultClass() {

	if( $('#posClassRadio').is(':checked') ) {
		defaultClass = 1;
	} else {
		defaultClass = -1;
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
//	Retreive the boundaries for nuclei within the viewport bounds.
//	TODO - Look into expanding the nuclei request to a 'viewport' width
//			boundary around the view port. Since we are now using the
//			'animation-finish' event to trigger the request, it may be
//			possible to retreive that many boundaries in a sufficient
//			amount of time
//
function updateSeg() {

	if (application == "region") {
		scale = 0.2;
	}

	if( statusObj.scaleFactor() > scale ) {

		var left, right, top, bottom, width, height;

		// Grab nuclei a viewport width surrounding the current viewport
		//
		width = statusObj.dataportRight() - statusObj.dataportLeft();
		height = statusObj.dataportBottom() - statusObj.dataportTop();

		left = (statusObj.dataportLeft() - width > 0) ?	statusObj.dataportLeft() - width : 0;
		right = statusObj.dataportRight() + width;
		top = (statusObj.dataportTop() - height > 0) ?	statusObj.dataportTop() - height : 0;
		bottom = statusObj.dataportBottom() + height;

	    $.ajax({
			type: "POST",
       	 	url: "db/getnuclei.php",
       	 	dataType: "json",
			data: { uid: 	uid,
					slide: 	curSlide,
					trainset: "PICKER",
					dataset: "none",
					left:	left,
					right:	right,
					top:	top,
					bottom:	bottom,
					application: application
			},

			success: function(data) {

					var ele;
					var segGrp = document.getElementById('segGrp');
					var annoGrp = document.getElementById('anno');

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
							ele.setAttribute('stroke', 'aqua');
							ele.setAttribute('fill', 'aqua');

							segGrp.appendChild(ele);
						}

					} else{
						for( cell in data ) {
							ele = document.createElementNS("http://www.w3.org/2000/svg", "polygon");

							ele.setAttribute('points', data[cell][0]);
							ele.setAttribute('id', 'N' + data[cell][1]);
							ele.setAttribute('stroke', 'aqua');
							if (application == "region") {
								ele.setAttribute('stroke-width', 4);
								ele.setAttribute("stroke-dasharray", "5,5");
							}
							ele.setAttribute('fill', 'none');

							segGrp.appendChild(ele);
						}

					}

					if( selectedJSON.length > 0 ) {
						updateBoundColors();
					}
        		}
    	});
	}
}



//
//	Adds another thumbnail div to the thumbnail slider
//
function addThumbnail(index) {

	var slider = document.getElementById('thumbSlider'),
		thumbDiv, ele, divEle;


	thumbDiv = document.createElement("div");
	thumbDiv.setAttribute('class', 'slider_div');
	thumbDiv.setAttribute('id', 'box_' + index);
	thumbDiv.setAttribute('style', 'display: none;');

	divEle = document.createElement("div");
	divEle.setAttribute('class', 'classLabel');

	ele = document.createElement("strong");
	ele.setAttribute('id', 'label_' + index);
	ele.innerHTML = "Class";
	divEle.appendChild(ele);
	thumbDiv.appendChild(divEle);

	divEle = document.createElement("div");

	ele = document.createElement("img");
	ele.setAttribute('id', 'thumb_' + index);
	ele.setAttribute('height', '100');
	ele.setAttribute('width', '100');
	divEle.appendChild(ele);
	thumbDiv.appendChild(divEle);

	slider.appendChild(thumbDiv);

	width = 0;
	$('#overflow .slider div').each(function() {
		width += $(this).outerWidth(true);
	});
	$('#overflow .slider').css('width', width + "px");

}





function nucleiSelect() {

	if( selectNuc ) {
		$.ajax({
	        type:   "POST",
            url:    "db/getsingle.php",
            dataType: "json",
            data:   { slide:    curSlide,
                      cellX:    statusObj.mouseImgX().toFixed(1),
                      cellY:    statusObj.mouseImgY().toFixed(1),
											application: application
            },
            success: function(data) {
					if( data !== null ) {

						var newBox = false;

						if( statusObj.totalSel() === 0 ) {
							// First object added, enable "add" button
							$('#addBtn').removeAttr('disabled');
						}

						// Need to add thumbnail box if we have 8 or more samples
						//
						if( statusObj.totalSel() >= 8 ) {
							addThumbnail(statusObj.totalSel() + 1);
							newBox = true;
						}

						sample = {};

						// Distance from nuclei is element 4
						sample['slide'] = curSlide;
						sample['id'] = data[1];
						sample['centX'] = data[2];
						sample['centY'] = data[3];
						sample['boundary'] = data[0];
						sample['maxX'] = data[4];
						sample['maxY'] = data[5];
						sample['scale'] = data[6];
						sample['label'] = defaultClass;
						sample['clickX'] = data[7];
						sample['clickY'] = data[8];

						// check if a sample selected is duplicated or not
						if (!duplicateCheck(sample['centX'], sample['centY'])){

						// Add the selected nuclei
						//
						statusObj.totalSel(statusObj.totalSel() + 1);
						selectedJSON.push(sample);

						if( newBox ) {
							var index = statusObj.totalSel();
							var	box = document.getElementById('box_'+index);
							var	clickCount = 0;

							box.addEventListener('click', function() {
										clickCount++;
										if( clickCount === 1 ) {
											singleClickTimer = setTimeout(function() {
												clickCount = 0;
												thumbSingleClick('box_'+index);
												}, 200);
										} else if( clickCount === 2 ) {
											clearTimeout(singleClickTimer);
											clickCount = 0;
											thumbDoubleClick('box_'+index);
										}
									}, false);
							boxes.push('box_'+index);
						}

						var box = "#box_" + statusObj.totalSel(), thumbTag = "#thumb_" + statusObj.totalSel(),
							labelTag = "#label_" + statusObj.totalSel(), loc;

						$(box).show();

						var scale_cent = 25;
						var scale_size = 50.0;

						if (application == "region"){
							scale_cent = 64;
							scale_size = 128;
						}
						centX = (sample['centX'] - (scale_cent * sample['scale'])) / sample['maxX'];
						centY = (sample['centY'] - (scale_cent * sample['scale'])) / sample['maxY'];
						sizeX = (scale_size * sample['scale']) / sample['maxX'];
						sizeY = (scale_size * sample['scale']) / sample['maxY'];
						loc = centX+","+centY+","+sizeX+","+sizeY;

						var thumbNail = IIPServer+"FIF="+pyramids[$('#slideSel').prop('selectedIndex')]+
										SlideLocPre+loc+"&WID=100"+SlideLocSuffix;

						$(thumbTag).attr("src", thumbNail);
						updateClassStatus(statusObj.totalSel() - 1);
						updateBoundColors();
					}	else {
					window.alert("Selected sample is duplicted !!");
					}

				}
			}
    	});
	}
}


//
// Check if a sample selected is duplicated or not
// Parameters
// centroid position (X,Y) of the selected sample
// Return
// true: if duplicated
// false: if not duplicated
//
function duplicateCheck(x,y) {

	var centX = x;
	var centY = y;

	for( i = 0; i < selectedJSON.length; i++ ) {
			if ((selectedJSON[i]['centX'] == centX) && (selectedJSON[i]['centY'] == centY))
			return true;
	}

	return false;
}


//
//	A single click in the thumbnail box toggles the current classification
//	of the object.
//
//
function thumbSingleClick(box) {

	var index = boxes.indexOf(box);
	var label = selectedJSON[index]['label'];

	if( label === 1 ) {
		selectedJSON[index]['label'] = -1;
	} else {
		selectedJSON[index]['label'] = 1;
	}
	updateClassStatus(index);
	if (application = "region"){
		updateBoundColors();
	}
};




//
//	A double click in the thumbnail box removes it from the list
//
//
function thumbDoubleClick(box) {

	var index = boxes.indexOf(box) + 1;
	var	src, dest;

	selectedJSON.splice(index - 1, 1);

	for(i = index; i < statusObj.totalSel(); i++) {

		dest = "#thumb_" + i;
		src = "#thumb_" + (parseInt(i) + 1);

		$(dest).attr("src", $(src).attr("src"))

		updateClassStatus(i - 1);
	}

	if( statusObj.totalSel() > 8 ) {

		boxes.pop();
		dest = document.getElementById("box_" + statusObj.totalSel());
		dest.parentNode.removeChild(dest);

	} else {
		dest = "#label_" + statusObj.totalSel();
		$(dest).text("Class");

		dest = "#box_" + statusObj.totalSel();
		$(dest).hide();
	}
	statusObj.totalSel(statusObj.totalSel() - 1)
	updateSeg();
};





function updateClassStatus(sample) {

	var labelTag = "#label_"+(parseInt(sample)+1),
		label = $('#box_'+(parseInt(sample)+1)).children(".classLabel");

	label.removeClass("negLabel");
	label.removeClass("posLabel");

	if( selectedJSON[sample]['label'] === 1 ) {
		$(labelTag).text(posClass);
		label.addClass("posLabel");
	} else {
		$(labelTag).text(negClass);
		label.addClass("negLabel");
	}
}




function updateBoundColors() {

	for( cell in selectedJSON ) {
		var bound = document.getElementById("N"+selectedJSON[cell]['id']);

		if( bound != null ) {
			if (application == "region"){
				if( selectedJSON[cell]['label'] === 1 ) {
					bound.setAttribute('stroke', 'yellow');
					bound.setAttribute('fill', 'yellow');
					bound.setAttribute("fill-opacity", "0.2");
				} else {
					bound.setAttribute('stroke', 'aqua');
					bound.setAttribute('fill', 'aqua');
					bound.setAttribute("fill-opacity", "0.2");
				}
			} else{
				bound.setAttribute('stroke', 'yellow');

			}
		}
	}
}




//
//	+++++++++++    Openseadragon mouse event handlers  ++++++++++++++++
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




//
//	The double click handler doesn't seem to work. So we create
//	our own with a timer.
//
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
		clickCount = 0;
		nucleiSelect();
	}
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





//
//	+++++++++++++	Button handlers +++++++++++++++++++++++++++++++++++++++++++
//

//
//	Set the display flag, update the segmentation button text and
//	set the visibility of the SVG element.
//
function showSegmentation() {

	var	segBtn = $('#segBtn');

	if( displaySeg ) {
		// Currently displaying segmentation, hide it
		segBtn.val("Show Segmentation");
		$('.overlaySvg').css('visibility', 'hidden');
		displaySeg = false;
	} else {
		// Segmentation not currently displayed, show it
		segBtn.val("Hide Segmentation");
		$('.overlaySvg').css('visibility', 'visible');
		displaySeg = true;

		updateSeg();
	}
}




//
//	Pass the selected nuclei to the active learning server and
//	start the "select / label" process.
//
function addObjects() {
	valid = true;

	for( i = 0; i < statusObj.totalSel(); i++ ) {

		if( selectedJSON[i]['label'] === 0 ) {

			window.alert("Need to set class for all selected objects");
			valid = false;
			break;
		}
	}

	if( valid ) {

		$.ajax({
			type: "POST",
			url: "php/addObjects.php",
			data: {samples: selectedJSON},
			dataType: "json",
			success: function(data) {
				if( data['status'] === "PASS" ) {
					var boxTag, labelTag;

					statusObj.testSetCnt(data['count']);

					// Cleanup grid
					for(i = 0; i < statusObj.totalSel(); i++ ) {
						boxTag = "#box_" + (parseInt(i) + 1);

						$(boxTag).hide();

						labelTag = "#label_"+(parseInt(i)+1);
						$(labelTag).text("Class");
					}

					for(i = statusObj.totalSel(); i > 8; i--) {
						// Remove any boxes that were added
						boxes.pop();
						var	dest = document.getElementById("box_" + i);
						dest.parentNode.removeChild(dest);
					}

					selectedJSON = [];
					statusObj.totalSel(0);
					$('#addBtn').attr('disabled', 'true');
					// We have objects added to the test set, enable save
					$('#saveBtn').removeAttr('disabled');
					// Now review is available
					$('#revBtn').removeAttr('disabled');

					// Refresh boundaries to highligh those just added
					updateSeg();

				} else {
					// TODO - Indicate failure
					console.log("Adding objects failed");
				}
			}
		});
	}
}




function saveTrainingSet() {

	if( reloaded ) {

		$.ajax({
			type: "POST",
			url: "php/finishReloadedPicker.php",
			data: "",
			dataType: "json",
			success: function(data) {

				if( data['status'] === "PASS" ) {
					console.log("Pos: "+data['posClass']+", Neg: "+data['negClass']);
					window.alert("Test set saved to: " + data['filename']);
					window.location = "validation.html?application="+application;
				} else {
					window.alert("Unable to save test set");
				}
			},
		});

	} else {

		$.ajax({
			type: "POST",
			url: "php/finishPicker.php",
			data: "",
			dataType: "json",
			success: function(data) {

				if( data['status'] === "PASS" ) {
					console.log("Pos: "+data['posClass']+", Neg: "+data['negClass']);
					window.alert("Test set saved to: " + data['filename']);
					window.location = "validation.html?application="+application;
				} else {
					window.alert("Unable to save test set");
				}
			},
		});

	}
}






//
// 	Toggles the nuclei selection process. A double click selects an object.
// 	The OpenSeadragon viewer also uses the double click to zoom in. Selecting
//	should be done while fully zoomed in and only if in the 'selection' mode.
//
function setSelectMode() {

	var	selBtn = $('#selBtn');
	if( selectNuc ) {
		// Currently selecting nuclei, stop
		selBtn.val("Select Nuclei");
		selBtn.css('color', 'white');
		selectNuc = false;
	} else {
		// Not currently selecting nuclei, start
		selBtn.val("Stop Selecting");
		selBtn.css('color', 'red');
		selectNuc = true;
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





function cancelSession() {

	$.ajax({
		url: "php/cancelSession.php",
		data: "",
		success: function() {
			window.location = "validation.html?application="+application;
		}
	});
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
	posSel: ko.observable(0),
	negSel: ko.observable(0),
	totalSel: ko.observable(0),
	testSetCnt: ko.observable(0)
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
