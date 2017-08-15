function ColorNormI = ColorNormalization_Tile(OriginalI,target_mean,target_std,M)
%Performs color normalization on a single tile
% Inputs:
% OriginalI - M x N x 3 rgb original image to be normalized.
% target_mean - mapping parameter, output of the TargetParameters function
% target_std - mapping parameter, output of the TargetParameters function
% M - a 4x2 matrix containing the Fisher's linear discriminant function coefficients.
% M =[-0.154 0.035 0.549 -45.718; -0.057 -0.817 1.170 -49.887];
% DiscriminantF_BG =  M(1,1)*R + M(1,2)*G + M(1,3)*B + M(1,4);
% DiscriminantF_FG =  M(2,1)*R + M(2,2)*G + M(2,3)*B + M(2,4);
% Output:
% ColorNormI - normalized image
    
   % Segmenting foreground from background using discriminant functions
        
     [r,c,d] = size(OriginalI);
     o_fg = SegFG(OriginalI,M);
     o_bg = imcomplement(o_fg);
   
   % Applying Reinhard's method   
   % Converting RGB signals to lab(l,alpha,beta) 
     [o_idx_fg,o_idx_bg,o_fg_lab,o_bg_lab] = PixelClass(OriginalI,o_fg,o_bg);
     
   % Mapping the color distribution of an image to that of the target image
     new_fg = transferI(o_fg_lab, target_mean, target_std);
     new_im=zeros(length(o_idx_fg)+length(o_idx_bg),3);
     new_im(o_idx_fg,:)=new_fg;
     new_im(o_idx_bg,:)=o_bg_lab;
    
   % Converting lab to RGB
     new_im = lab2rgb(new_im);  
     ColorNormI = reshape(uint8(new_im*255), r,c,d);
    
end