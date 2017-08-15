#include <openslide.h>
#include "mex.h"

/*Implements a Matlab mex-wrapper for openslide to provide excraction of 	*/
/*multiple regions from a single slide, at possibly different resolutions.	*/
/*This interface was developed since it is difficult to maintain persistence*/
/*of a C++ struct between calls to MEX functions. This interface allows the */
/*user to retrieve multiple regions from a slide, avoiding repeated calls to*/
/*openslide_open() and the accompanying over head. The user can first call  */
/*openslide_check_levels() to get a summary of the levels present, their    */
/*dimensions and magnifications. Then this information can be used to 		*/ 
/*determine which areas to extract.											*/
/*The user passes in (level, x, y, h, w) tuples which are each used with 	*/
/*openslide_read_region() to generate an RGB image in uint8 planar format 	*/
/*(as Matlab expects it). These images are returned in the cells of a cell 	*/
/*array.*/
/*inputs:																	*/	
/*File - string containing filename and path								*/
/*level - n-length vector of doubles containing levels for each region to be*/
/*        extracted.														*/
/*x - n-length vector of doubles containing the horizontal coordinate of the*/
/*	  region upper left corner.												*/
/*y - n-length vector of doubles containing the vertical coordinate of the	*/
/*	  region upper left corner.												*/
/*h - n-length vector of doubles containing height of region to be extracted	*/
/*w - n-length vector of doubles containing width of region to be extracted*/
/*outputs:																	*/
/*Images - an n-length cell array with each cell containing the extracted   */
/*		   regions. Regions are in uint8 format, in a planar configuration  */
/*		   with R, G, then B, as Matlab function expect them.				*/
 
size_t IsVector(const mxArray *p) {
/*used to determine if input is vector, and return length if true*/
	
	size_t m, n;
	m = mxGetM(p);
	n = mxGetN(p);
	if(n != 1) {
		if(m != 1) {
			return 0;
		}
		else {
			return n;
		}
	}
	else {
		return m;
	}
}

void ARGBtoRGB(uint32_t *ARGB, size_t w, size_t h, unsigned char *RGB) {
/*converts interlaced ARGB image array to planar configuration RGB image in uint8 format*/

	/*variables*/
	size_t i, j; /*loop iterators*/
	uint32_t *input; /*temp pointer to input ARGB*/
	unsigned char *outputR, *outputG, *outputB; /*temp pointers to planes of output RGB*/

	/*copy starting location of arrays*/
	input = ARGB;
	outputR = RGB;
	outputG = outputR+w*h;
	outputB = outputG+w*h;

	/*convert each pixel - input is row-major, output is column-major*/
	for(i = 0; i < h; i++) {
		for(j = 0; j < w; j++) {
			*(outputR + j*h + i) = (unsigned char)((*input & 0x00ff0000) >> 16);
			*(outputG + j*h + i) = (unsigned char)((*input & 0x0000ff00) >> 8);
			*(outputB + j*h + i) = (unsigned char)(*input & 0x000000ff);
			input++;
		}
	}
}
 
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

	/*variables*/
	char *buffer; /*array for input string*/
	int32_t levels; /*number of levels*/
	uint32_t *image; /*buffer for RGBA extracted region*/
	unsigned char *RGB; /*buffer for RGB corrected region*/
	size_t llevel, lx, ly, lw, lh; /*input argument lengths*/
	double *level, *x, *y, *w, *h; /*pointers to input arguments*/
	openslide_t *slide; /*openslide struct*/
	mwSize RGBdims[3]; /*dimensions for output image*/
	mwSize outputdims[2]; /*dimensions for output cell array*/
	mxArray *swap; /*temporary pointer to capture function output*/
	size_t i; /*loop iterator*/
	
	/*check input arguments*/
	if(nrhs != 6) {
		mexErrMsgTxt("'openslide_read_regions.m' requires one input argument.");
	}
	if(nlhs > 1) {
		mexErrMsgTxt("'openslide_read_regions.m' produces one output.");
	}

	/*get lengths of input arguments*/
	llevel = IsVector(prhs[1]);
	lx = IsVector(prhs[2]);
	ly = IsVector(prhs[3]);
	lh = IsVector(prhs[4]);
	lw = IsVector(prhs[5]);
	
	/*check input 1 type*/
	if(!mxIsChar(prhs[0]) || (mxGetM(prhs[0]) != 1)) {
		mexErrMsgTxt("Input 1 must be character array.");
	}

	/*copy input 1 to character array*/
	buffer = mxArrayToString(prhs[0]);
	
	/*check inputs 2-end type*/
	if(!mxIsDouble(prhs[1]) || !(llevel > 0)) {
		mexErrMsgTxt("Input 2 must be a vector of doubles.");
	}
	if(!mxIsDouble(prhs[2]) || !(llevel > 0)) {
		mexErrMsgTxt("Input 3 must be a vector of doubles.");
	}	
	if(!mxIsDouble(prhs[3]) || !(llevel > 0)) {
		mexErrMsgTxt("Input 4 must be a vector of doubles.");
	}
	if(!mxIsDouble(prhs[4]) || !(llevel > 0)) {
		mexErrMsgTxt("Input 5 must be a vector of doubles.");
	}
	if(!mxIsDouble(prhs[5]) || !(llevel > 0)) {
		mexErrMsgTxt("Input 6 must be a vector of doubles.");
	}
	
	/*check for consistent lengths of inputs 2 - 6*/
	if(llevel != lx || lx != ly || ly != lw || lw != lh) {
		mexErrMsgTxt("Inputs 2 through 6 must have same length");
	}
	
	/*get pointers to inputs*/
	level = mxGetPr(prhs[1]);
	x = mxGetPr(prhs[2]);
	y = mxGetPr(prhs[3]);
	h = mxGetPr(prhs[4]);
	w = mxGetPr(prhs[5]);
	
	/*generate output*/
	if(mxGetM(prhs[1]) > mxGetN(prhs[1])) {
		outputdims[0] = llevel;
		outputdims[1] = 1;
	}
	else {
		outputdims[0] = 1;
		outputdims[1] = llevel;
	}
	plhs[0] = mxCreateCellArray(2, outputdims);
	
	/*copy input*/
	if(openslide_can_open(buffer) == 1) {
			
		/*open*/	
		slide = openslide_open(buffer);

		/*check for error*/
		if(openslide_get_error(slide) != NULL) {
			mexErrMsgTxt("'openslide_read_regions.m' cannot open slide.");
		}
		else { /*iterate through region list, extracting each*/
			
			/*get # of levels*/
			levels = openslide_get_layer_count(slide);
			
			/*iterate through user requests*/
			for(i = 0; i < llevel; i++) {
			
				/*level limit check*/
				if(level[i] >= levels) {
					mexErrMsgTxt("Level index exceeded.");
				}
			
				/*allocate memory for RGBA image*/
				image = (uint32_t*)malloc(w[i]*h[i]*sizeof(uint32_t));
				
				/*allocate memory for RGB output image*/
				RGBdims[0] = h[i];
				RGBdims[1] = w[i];
				RGBdims[2] = 3;
				swap = mxCreateNumericArray(3, RGBdims, mxUINT8_CLASS, mxREAL);
				RGB = (unsigned char*)mxGetPr(swap);
			
				/*check allocation*/
				if(image == NULL || RGB == NULL) {
					mexErrMsgTxt("Could not allocate RGBA/RGB arrays for openslide_read_region");
				}
				else {
					/*get region*/
					openslide_read_region(slide, image, x[i], y[i], level[i], w[i], h[i]);
					
					/*strip out alpha channel*/
					ARGBtoRGB(image, (size_t)w[i], (size_t)h[i], RGB);
					
					/*copy to cell array output*/
					mxSetCell(plhs[0], i, swap);
					
				}
				
				/*free RGBA image*/
				free(image);
				
			}
		}
	}
	else{ /*can't open slide*/
		mexErrMsgTxt("'openslide_read_regions.m' cannot open slide.");
	}
	
	/*free input buffer copy*/
	mxFree(buffer);
	
}
