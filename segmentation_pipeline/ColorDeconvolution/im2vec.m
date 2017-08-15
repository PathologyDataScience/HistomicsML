function vec = im2vec(I)
%converts color image to 3 x MN matrix

M = size(I,1);
N = size(I,2);

if(size(I,3) == 3)
    vec = [I(1:M*N); I(M*N+1:2*M*N); I(2*M*N+1:3*M*N)];
elseif(size(I,3) == 2)
    vec = [I(1:M*N); I(M*N+1:2*M*N)];
elseif(size(I,3) == 1)
    vec = [I(:)];
else
    vec = [];
end