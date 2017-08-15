#include <openslide.h>
#include "mex.h"

/*Implements a Matlab mex-wrapper for openslide to provide access to the 	*/
/*function 'openslide_can_open'							*/
/*compile - mex -L/opt/lib/ -lopenslide -I/opt/include/openslide/ openslide_can_open.C		*/
/*Success = openslide_check_levels(File);					*/
/*inputs:									*/	
/*File - string containing filename and path					*/
/*outputs:									*/
/*Success - double scalar indicating whether file can be opened (1) or not  	*/
/*	    (0).								*/

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

	/*variables*/
	char *buffer; /*array for input string*/
	bool result; /*output*/

	/*check input arguments*/
	if(nrhs != 1) {
		mexErrMsgTxt("'openslide_can_open.m' requires one input argument.");
	}
	if(nlhs > 1) {
		mexErrMsgTxt("'openslide_can_open.m' produces one output.");
	}
	
	/*check input type*/
	if(!mxIsChar(prhs[0]) || (mxGetM(prhs[0]) != 1)) {
		mexErrMsgTxt("Input must be character array.");
	}
	
	/*copy*/
	buffer = mxArrayToString(prhs[0]);
	
	/*copy input*/
	result = openslide_can_open(buffer);
	
	/*set output*/
	plhs[0] = mxCreateDoubleScalar((double)result);

	/*free input buffer copy*/
	mxFree(buffer);
}
