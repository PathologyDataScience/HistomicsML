function new_im = transferI(im, meanT,stdT)
 
% Map the color distribution of an image to that of the target image
% Reinhard et al. Color Transfer between Images, IEEE Computer Graphics and Application,2001
%inputs:
%im - lab(l,alpha,beta)signals for mapping
%MeanT - scalar mean parameter for mapping.
%StdT - scalar variance parameter for mapping.
%output:
%new_im - mapped(transfered) lab sginals
 
% remove NaN and -Inf
mask1 = ~isnan(im) & (im~=-Inf);
mask1 = mask1(:,1).*mask1(:,2).*mask1(:,3);
valid_im = im(find(mask1>0),:);
 
meanI = mean(valid_im);
stdI= std(valid_im);
 
% scale the data
[N,d] = size(valid_im);
new_im = valid_im - repmat(meanI, N, 1);
new_im = repmat(stdT./stdI, N, 1).*new_im;
new_im = new_im + repmat(meanT, N, 1); 
 
end