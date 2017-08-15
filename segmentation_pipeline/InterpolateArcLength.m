function [iX iY] = InterpolateArcLength(X, Y, L)
%Resamples the boundary points [X, Y] at L total equal arc-length
%locations.
%inputs:
%X - a K-length vector of horizontal coordinates.
%Y - a K-length vector of vertical coordinates.
%L - scalar, number of points to resample along.
%outputs:
%iX - L-length vector of horizontal interpolated coordinates with equal 
%     arc-length spacing.
%iY - L-length vector of vertical interpolated coordinates with equal 
%     arc-length spacing.

%flip
if(size(X,1) > size(X,2))
    X = X.';
    Y = Y.';
end

%get length of inputs
K = length(X);

%generate spaced points
Interval = [0:1/L:1];

%get segment lengths
Lengths = sqrt(diff(X).^2 + diff(Y).^2);

%normalize to unit length
Lengths = Lengths / sum(Lengths);

%calculate cumulative length along boundary
Cumulative = [0 cumsum(Lengths)];

%place points in 'Interval' along boundary
[~,Locations] = histc(Interval, Cumulative);

%clip to ends
Locations(Locations < 1) = 1;
Locations(Locations >= K) = K-1;

%linear interpolation
Lie = (Interval - Cumulative(Locations)) ./ Lengths(Locations);
iX = X(Locations) + (X(Locations+1) - X(Locations)) .* Lie;
iY = Y(Locations) + (Y(Locations+1) - Y(Locations)) .* Lie;

%flip
if(size(X,1) > size(X,2))
    iX = iX.';
    iY = iY.';
end