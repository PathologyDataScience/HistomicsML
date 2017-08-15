function [Mean Std] = TargetParameters(TargetI, M)
% Calculates mapping parameters for use in color normalization.
%inputs:
%TargetI - M x N x 3 rgb target image for color normalization.
%M - a 4x2 matrix containing the Fisher's linear discriminant function coefficients. 
  % DiscriminantF_BG =  M(1,1)*R + M(1,2)*G + M(1,3)*B + M(1,4);
  % DiscriminantF_FG =  M(2,1)*R + M(2,2)*G + M(2,3)*B + M(2,4);
%ouputs:
%Mean - scalar mean parameter for mapping.
%Std - scalar variance parameter for mapping.

% Target image processing
  % Segmenting foreground from background using discriminant functions
    if nargin == 1
        M =[-0.154 0.035 0.549 -45.718; -0.057 -0.817 1.170 -49.887];
    end
    t_fg = SegFG(TargetI,M);
    
  % Converting RGB signals of the segmented foreground to Runderman et al.'s perception-based color space lab(l,alpha,beta)  
    t_idx_fg=find(t_fg == 1);
    [r,c,d] = size(TargetI);
    tar = reshape(double(TargetI), r*c, d);
    tar_fg=tar(t_idx_fg,:);
    t_fg_rgb = tar_fg/255;
    t_fg_lab = rgb2lab(t_fg_rgb);
    
  % Select pixels
    mask2 = ~isnan(t_fg_lab) & (t_fg_lab~=-Inf);
    mask2 = mask2(:,1).*mask2(:,2).*mask2(:,3);
    t_fg_lab = t_fg_lab(find(mask2>0),:);
    
  % Calculate mapping parameters
    Mean = mean(t_fg_lab);
    Std = std(t_fg_lab);