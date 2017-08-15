function [XClean YClean] = DeleteDuplicateVertices(X, Y)
%Merges consecutive colinear segments in polyline contour with vertices 'X'
%and 'Y'.  This is useful for reducing computation time for detection of
%self-intersection.
%inputs:
%X - N-length vector of polyline contour vertex x-coordinates.
%Y - N-length vector of polyline contour vertex y-coordinates.
%outputs:
%XMerged - N-length vector of polyline contour vertex x-coordinates with 
%          colinear segments merged.
%YMerged - N-length vector of polyline contour vertex y-coordinates with 
%          colinear segments merged.

%initialize outputs
XClean = X;
YClean = Y;

%identify unique x-coordinates
[uX] = unique(X);

%sweep-line in x direction
for i = 1:length(uX)
    
    %find coordinates with shared X vertex 'uX(i)'
    Xhits = find(X == uX(i));
    
    if(length(Xhits) > 1)
        %initialize deletion array
        delete = false(size(Xhits));
        
        %identify Y coordinates associated with vertices 'Xhits'
        Ys = Y(Xhits);
        
        %sort Ys to find repeats
        [uY m n] = unique(Ys);
            
        %find repeated elements of Ys
        for j = 1:length(uY)

            %find elements of Ys equal to uY(j)
            Yhits = (uY(j) == Ys);
            
            if (sum(Yhits) > 1)
                delete = delete | Yhits;
            end            
        end
    end
end