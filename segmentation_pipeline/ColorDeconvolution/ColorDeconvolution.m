function [Intensity Complemented] = ColorDeconvolution(I, M, stains, complement)
%Deconvolves multiple stains from RGB image I, given stain matrix 'M'.
%inputs:
%I - an m x n x 3 RGB image in uint8 form range [0 - 255].
%M - a 3x3 matrix containing the color vectors in columns. For two stain images
%    the third column is zero.  Minumum two nonzero columns required.
%stains - a logical 3-vector indicating which output channels to produce.
%           Each element corresponds to the respective column of M.  Helps
%           reduce memory footprint when only one output stain is required.
%complement - binary flag indicating whether complementation is necessary
%output:
%intensity - an m x n x sum(stains) intensity image in single precision format.  Each
%         channel is a stain intensity.  Channels are ordered same as
%         columns of 'M'.
%Compelemented - the complemented 3x3 version of 'M'.

%example inputs:
% M =[0.650 0.072 0;  %H&E 2 color matrix
%  0.704 0.990 0;
%  0.286 0.105 0];
%M = [0.29622945 0.47740915 0.72805583; %custom user H&E
%     0.82739717 0.7787432 0.001;
%    0.4771394  0.40698838 0.6855175];

if nargin == 3 %check arguments
    complement = false;
end

for i = 1:3 %normalize stains
   if(norm(M(:,i), 2) >= eps)
       M(:,i) = M(:,i)/norm(M(:,i));
   end
end

if(complement || norm(M(:,3)) < eps) %only two colors specified
    M = ComplementStains(M);
    Complemented = M;
else
    Complemented = [];
end

Q = inv(M); %inversion
Q = single(Q(logical(stains),:));

dn = DeconvolutionNormalize(single(im2vec(I))+eps); %transform

cn = Q * dn; %unmix

channels = DeconvolutionDenormalize(cn); %invert transformation

m = size(I,1); n = size(I,2); %format output
Intensity = single(zeros(m, n, sum(stains)));
for i = 1:sum(stains)
   Intensity(:,:,i) = reshape(channels(i,:), [m n]);
end