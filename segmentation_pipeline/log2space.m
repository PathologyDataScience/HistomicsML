function y = log2space(a,b,n)
%Returns n logarithmically-spaced points from 2^a to 2^b.

y = (2).^ [a+(0:n-2)*(b-a)/(n-1), b];
