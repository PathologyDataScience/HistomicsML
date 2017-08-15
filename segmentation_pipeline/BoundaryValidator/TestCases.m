function TestCases()

%repeated point on colinear segment
x1 = [0 1 1 1 2];
y1 = [1 1 1 1 1];
figure; plot(x1, y1, 'b.-'); hold on;
[x1p y1p] = MergeColinear(x1, y1);
plot(x1p, y1p, 'ro');

%repeated corner point
x2 = [0 1 1 1];
y2 = [1 1 1 0];
figure; plot(x2, y2, 'b.-'); hold on;
[x2p y2p] = MergeColinear(x2, y2);
plot(x2p, y2p, 'ro');

%narrow neck
x3 = [0 1 2 3 4 4 4 3 2 2 3 4 4 4 3 2 1 0 0 0 1 2 2 1 0 0 0];
y3 = [0 0 0 0 0 1 2 2 2 3 3 3 4 5 5 5 5 5 4 3 3 3 2 2 2 1 0];
figure; plot(x3, y3, 'b.-'); hold on;
[x3p y3p] = MergeColinear(x3, y3);
plot(x3p, y3p, 'ro');

%colinear spur
x4 = [0 2 1];
y4 = [0 0 0];
figure; plot(x4, y4, 'b.-'); hold on;
[x4p y4p] = RemoveSpurs(x4, y4);
plot(x4p, y4p, 'ro');

%non-colinear spur 'T'-shaped
x5 = [0 1 1 1 2];
y5 = [0 0 1 0 0];
figure; plot(x5, y5, 'b.-'); hold on;
[x5p y5p] = RemoveSpur(x5, y5);
plot(x5p, y5p, 'ro');

%non-colinear spur, '+' shaped
x6 = [1 0 1 1 1 2 1 1 1];
y6 = [1 1 1 0 1 1 1 2 1];
figure; plot(x6, y6, 'b.-'); hold on;
[x6p y6p] = RemoveSpur(x6, y6);
plot(x6p, y6p, 'ro');

end