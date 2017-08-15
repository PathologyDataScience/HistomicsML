function [L, bX, bY] = Individual(seg_big)
%Segmentation of foreground image into label image to label individual cells.
%inputs:
%seg_big - .
%outputs:
%L - T x T label image.
%bX - K-length cell array containing horizontal object boundaries
%bY - K-length cell array containing vertical object boundaries

distance = -bwdist(~seg_big);
distance(~seg_big) = -Inf;
distance2 = imhmin(distance, 1);

clear distance

seg_big(watershed(distance2) == 0) = 0;
seg_nonoverlap = seg_big;

clear lines distance2

L = bwlabel(seg_nonoverlap, 4);
stats = regionprops(L, 'Area');
areas = [stats.Area];
ind = find(areas>20 & areas<1000);

seg = ismember(L,ind);
seg = imfill(seg, 'holes');
L = bwlabel(seg,4);
Bounds = bwboundaries(seg, 4, 'noholes');

bX = cellfun(@(x)x(:,2), Bounds, 'UniformOutput', false);
bY = cellfun(@(x)x(:,1), Bounds, 'UniformOutput', false);