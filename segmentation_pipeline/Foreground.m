function seg_big = Foreground(color_img, T1, T2, G1, G2)
%Foreground/background segmentation method.
%inputs:
%color_img - T x T x 3 color image.
%T1 - scalar, recommended 5
%T2 - scalar, recommended 4
%G1 - scalar, recommended 80
%G2 - scalar, recommended 45
%outputs:
%seg_big - T x T background image.

%define inputs if undefined
if(nargin == 1)
	T1=5; T2=4; G1=80; G2=45;
end

%get color channels
r = color_img( :, :, 1);
g = color_img( :, :, 2);
b = color_img( :, :, 3);

%strip out red blood cells
imR2G = double(r)./(double(g)+eps);
bw1 = imR2G > T1;
bw2 = imR2G > T2;
ind = find(bw1);
if ~isempty(ind)
    [rows, cols]=ind2sub(size(imR2G),ind);
    rbc = bwselect(bw2,cols,rows,8) & (double(r)./(double(b)+eps)>1);
else
    rbc = zeros(size(imR2G));
end

%morphological reconstruction
rc = 255 - r;
rc_open = imopen(rc, strel('disk',10));
rc_recon = imreconstruct(rc_open,rc);
diffIm = rc-rc_recon;

%postprocessing, fill holes and filter on area
bw1 = imfill(diffIm>G1,'holes');
L = bwlabel(bw1, 8);
stats = regionprops(L, 'Area');
areas = [stats.Area];
ind = find(areas>10 & areas<1000);
bw1 = ismember(L,ind);
ind = find(bw1);

if(isempty(ind)) 

    seg_big = false(size(bw1));
    return;

else

    bw2 = diffIm>G2;
    [rows,cols] = ind2sub(size(diffIm),ind);
    seg_norbc = bwselect(bw2,cols,rows,8) & ~rbc;
    seg_nohole = imfill(seg_norbc,'holes');
    seg_open = imopen(seg_nohole,strel('disk',1));
    seg_big = imdilate(bwareaopen(seg_open,30),strel('disk',1));

end