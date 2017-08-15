function [XMerged YMerged] = MergeColinear(X, Y)
%Merges consecutive colinear segments in polyline contour with vertices 'X'
%and 'Y'.  This is useful for reducing computation time for detection of
%self-intersection.
%inputs:
%X - N-length vector of polyline contour vertex x-coordinates.
%Y - N-length vector of polyline contour vertex y-coordinates.
%outputs:
%XMerged - N-length vector of polyline contour vertex x-coordinates with 
%          colinear segments merged.
%YMerged - N-length vector of polyline contour vertex y-coordinates with 
%          colinear segments merged.

%compute differentials
dX = diff(X);
dY = diff(Y);

%detect and delete stationary repeats
repeats = find((dX == 0) & (dY == 0));
X(repeats) = []; Y(repeats) = [];
dX(repeats) = []; dY(repeats) = [];

%calculate slope
slope = dY ./ dX;

%find slope transitions
dslope = diff(slope);
dslope(isnan(dslope)) = 0;

%identify slope transitions
transitions = find(dslope ~= 0);

%build merged sequences
XMerged = X(1);
YMerged = Y(1);
for i = 1:length(transitions)
    XMerged = [XMerged X(transitions(i)+1)];
    YMerged = [YMerged Y(transitions(i)+1)];
end
XMerged = [XMerged X(end)];
YMerged = [YMerged Y(end)];

end