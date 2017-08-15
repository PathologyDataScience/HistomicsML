function F = FSDs(X,Y,K,Intervals)
%Calculated FSDs from boundary points X,Y. Boundaries are resampled to have
%K equally spaced points (arclength) around the shape. The curvature is
%calculated using the cumulative angular function, measuring the
%displacement of the tangent angle from the starting point of the boundary.
%The K-length fft of the cumulative angular function is calculated, and
%then the elements of 'F' are summed as the spectral energy over
%'Intervals'.
%inputs:
%X - L-length vector of horizontal boundary coordinates.
%Y - L-length vector of vertical boundary coordinates.
%K - scalar, # of points to use in resampling boundary. Suggested power of
%    two.
%Intervals - d-length vector indicating intervals over which to sum
%            spectral energy. Sums go from 1:Intervals(1), Intervals(1)+1:
%            Intervals(2), etc. A value of Intervals = 1:K gives the entire
%            Spectral energy.
%outputs:
%F - length(Intervals) vector containing spectral energy of cumulative 
%    angular function, summed over defined 'Intervals'.

%check input 'Intervals'
if(Intervals(1) ~= 1)
    Intervals = [1 Intervals];
end
if(Intervals(end) ~= K/2)
    Intervals = [Intervals K];
end

%generate arc-length intervals for curvature calculation
I = (0:1/K:1);

%interpolate boundaries
[iX iY] = InterpolateArcLength(X, Y, K);

%calculate curvature
Curvature = atan2((iY(2:end) - iY(1:end-1)), (iX(2:end) - iX(1:end-1)));

%make curvature cumulative
Curvature = Curvature-Curvature(1);

%calculate FFT
fX = fft(Curvature);

%spectral energy
fX = fX .* conj(fX);
fX = fX / sum(fX);

%calculate 'F' values
F = zeros(1,length(Intervals)-1);
for i = 1:length(Intervals)-1
    F(i) = sum(fX(Intervals(i):Intervals(i+1)));
end