function Complemented = ComplementStains(M)
%Used in color deconvolution to complement the stains with a unit-norm
%orthogonal component.
%inputs:
%M - 3x3 matrix of calibrated stains (in columns).
%outputs:
%Compelemented - 3x3 matrix with column three adjusted to complement the
%                first and second stains.


%copy input to Complemented
Complemented = M;

%complement
if ((M(1,1)^2 + M(1,2)^2) > 1)
    Complemented(1,3) = 0;
else
    Complemented(1,3) = sqrt(1 - (M(1,1)^2 + M(1,2)^2));
end

if ((M(2,1)^2 + M(2,2)^2) > 1)
    Complemented(2,3) = 0;
else
    Complemented(2,3) = sqrt(1 - (M(2,1)^2 + M(2,2)^2));
end

if ((M(3,1)^2 + M(3,2)^2) > 1)
    Complemented(3,3) = 0;
else
    Complemented(3,3) = sqrt(1 - (M(3,1)^2 + M(3,2)^2));
end

Complemented(:,3) = Complemented(:,3)/norm(Complemented(:,3));
