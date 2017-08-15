function normalized = DeconvolutionNormalize(data)
%Normalize raw color values according to Rufriok and Johnston's color
%deconvolution scheme.
%inputs:
%data - 3 x N matrix of raw color vectors (type double or single)
%outputs:
%normalized - 3 x N matrix of normalized color vectors (type double or
%single)

normalized = -(255*log(data/255))/log(255);