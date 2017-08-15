function [xb yb] = CrustPolygonalReconstruction(X, Y)

numpts = length(X);

dt = DelaunayTri(X,Y);
tri = dt(:,:);
% Insert the location of the Voronoi vertices into the existing
% triangulation
V = dt.voronoiDiagram();
% Remove the infinite vertex
V(1,:) = [];
numv = size(V,1);
dt.X(end+(1:numv),:) = V;

% The Delaunay edges that connect pairs of sample points represent the
% boundary.
delEdges = dt.edges();
validx = delEdges(:,1) <= numpts;
validy = delEdges(:,2) <= numpts;
boundaryEdges = delEdges((validx & validy), :)';
xb = X(boundaryEdges);
yb = Y(boundaryEdges);
%clf;
%triplot(tri,X,Y);
%axis equal;
%hold on;
plot(xb,yb, '-k');
xlabel('Curve reconstruction from a point cloud', 'fontweight','b');
hold off;