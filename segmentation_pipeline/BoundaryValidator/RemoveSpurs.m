function [XClean YClean] = RemoveSpurs(X, Y)
%removes 'spurs' from polyline, defined as two sequential polyline segments
%that overlap (Ex. x = [0 2 1], y = [0 0 0] or x = [0 1 1 1 0], 
%y = [0 0 1 0 2].)
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

%detect segments where slope(i+1) = slope(i)
spurs = find((slope(1:end-1) == -slope(2:end)) & ...
        (sign(dX(1:end-1)) == -sign(dX(2:end))) & ...
        (sign(dY(1:end-1)) == -sign(dY(2:end))) );

%spurs = find(((slope(1:end-1) == -slope(2:end)) & ~(slope(1:end-1) == 0))...
%        | (dY(1:end-1) == 0 & dY(2:end) == 0 & dX(1:end-1) == -dX(2:end)));

%remove offending vertices
X(spurs+1) = [];
Y(spurs+1) = [];

%copy for output
XClean = X;
YClean = Y;

end