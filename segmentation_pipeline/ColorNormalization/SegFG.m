function I_seg_fg = SegFG(I,M)
% Segment foreground from background using discriminant functions
%inputs:
%I - M x N x 3 rgb image for color normalization.
%M - a 4x2 matrix containing the Fisher's linear discriminant function coefficients. 
  % DiscriminantF_BG =  M(1,1)*R + M(1,2)*G + M(1,3)*B + M(1,4);
  % DiscriminantF_FG =  M(2,1)*R + M(2,2)*G + M(2,3)*B + M(2,4);
%ouput:
%I_seg_fg - segmented binary image

discriminant1 = M(1,1)*double(I(:,:,1)) + M(1,2)*double(I(:,:,2)) + M(1,3)*double(I(:,:,3)) + M(1,4);
discriminant2 = M(2,1)*double(I(:,:,1)) + M(2,2)*double(I(:,:,2)) + M(2,3)*double(I(:,:,3)) + M(2,4);

I_seg_fg = discriminant1 < discriminant2;

end