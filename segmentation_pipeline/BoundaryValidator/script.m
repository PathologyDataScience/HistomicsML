close all; clear all; clc;

Filename = '\\storage01.cci.emory.edu\vg2.repos.data\Annotations\3+ Angiogenesis\TCGA-02-0107-01A-01-BS1.xml';

%load and parse xml annotation
Annotation = AperioXMLRead(Filename);
[~, Filename] = fileparts(Filename); %truncate for finding tiles below

%for each annotated region (vessel)
for i = 100:100%length(Annotation.Layers(1).Regions)
    
    %visualize
    figure;
    plot(Annotation.Layers(1).Regions(i).Coordinates(:,1),...
    Annotation.Layers(1).Regions(i).Coordinates(:,2), 'b.-'); hold on; axis equal;
%     plot(Annotation.Layers(1).Regions(i).Coordinates(1,1),...
%         Annotation.Layers(1).Regions(i).Coordinates(1,2), 'gp', 'MarkerSize', 14);
%     plot(Annotation.Layers(1).Regions(i).Coordinates(end,1),...
%         Annotation.Layers(1).Regions(i).Coordinates(end,2), 'kp', 'MarkerSize', 14);

    %extract and round region coordinates
    X = round(Annotation.Layers(1).Regions(i).Coordinates(:,1));
    Y = round(Annotation.Layers(1).Regions(i).Coordinates(:,2));

    %close polygon
%     if(X(1) ~= X(end) | Y(1) ~= Y(end))
%         X = [X; X(1)];
%         Y = [Y; Y(1)];
%     end
    
    %merge colinear segments
    [X Y] = MergeColinear(X, Y);

    %visualize
    plot(X, Y, 'go');
    
    %delete duplicate points
    [X Y] = DeleteDuplicateVertices(X, Y);
    
    %remove "spurs"
    [X Y] = RemoveSpurs(X, Y);

    %visualize
    plot(X, Y, 'rx-');
    
    %delaunay triangulation
    subplot(1,2,2);
    SelfConstrainedDelaunay(X.', Y.');
%     [xb yb] = CrustPolygonalReconstruction(X.', Y.');
%     figure; plot(xb, yb, 'b.-'); hold on;
%     plot(xb(1), yb(1), 'gs'); plot(xb(end), yb(end), 'rs');
      
    %submit to db - check errors
    %result = urlread('http://europa.cci.emory.edu:8080/pais/validate', 'post', {'coords', mat2str([X Y])});
end