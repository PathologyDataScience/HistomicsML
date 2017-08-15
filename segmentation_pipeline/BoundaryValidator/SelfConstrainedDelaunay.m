function SelfConstrainedDelaunay(X, Y)

N = length(X);

C = [(1:(N-1))' (2:N)'; N 1];

dt = DelaunayTri(X, Y, C);
io = dt.inOutStatus();
patch('faces',dt(io,:), 'vertices', dt.X, 'FaceColor','r');
axis equal;
xlabel('Constrained Delaunay Triangulation of usapolygon', 'fontweight','b');