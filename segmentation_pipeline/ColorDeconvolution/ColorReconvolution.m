function Color = ColorReconvolution(Stains, M)
%Reconvolves the stains obtained by 'ColorDeconvolution.m' into a color
%image, given the unmixing parameters.
%inputs:
%Stains - an m x n x 3 stain image in uint8 form, obtained from 'ColorDeconvolution'.
%M - a 3x3 matrix containing the color vectors in columns. Columns are
%    ordered consistently with channels of 'Stains'. This matrix must be 
%output:
%Color - a remixed m x n x 3 RGB image in uint8 form range [0 - 255].

for i = 1:3 %normalize stains
   if(norm(M(:,i), 2) >= eps)
       M(:,i) = M(:,i)/norm(M(:,i));
   end
end

dn = DeconvolutionNormalize(single(im2vec(Stains))); %transform

cn = M * dn; %remix

channels = DeconvolutionDenormalize(cn); %invert transformation

m = size(Stains,1); n = size(Stains,2); %format output
Color = uint8(zeros(m, n, 3));
for i = 1:3
   Color(:,:,i) = reshape(uint8(channels(i,:)), [m n]);
end