function denormalized = DeconvolutionDenormalize(data)
%de-normalize raw color values according to Rufriok and Johnston's color
%deconvolution scheme.
%inputs:
%data - 3 x N matrix of raw color vectors (type double or single)
%outputs:
%denormalized - 3 x N matrix of normalized color vectors (type double or
%single)

denormalized = exp(-(data - 255)*log(255)/255);