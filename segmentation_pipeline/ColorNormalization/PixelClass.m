function [idx_fg,idx_bg,fg_lab,bg_lab] = PixelClass(Img,fg, bg)
% Convert  RGB to lab(l,alpha,beta), each class(foregournd, background)
% Reinhard et al. Color Transfer between Images, IEEE Computer Graphics and Application,2001
%input:
%Img - Original image
%fg - RGB sginals(foreground)
%bg - RGB signals(backgorund)
%output:
%idx_fg - indices of foreground pixels
%idx_bg - indices of background pixels
%fg_lab - converted lab sginals(foreground)
%bg_lab - converted lab signals(backgorund)

    % foreground : tissue
    idx_fg=find(fg == 1);    
    [r,c,d] = size(Img);
    Img1 = reshape(double(Img), r*c, d);
        
    Img_fg=Img1(idx_fg,:);
    fg_rgb = Img_fg/255; 
    fg_lab = rgb2lab(fg_rgb);

    % background :  non-tissue
    idx_bg=find(bg == 1);
    Img_bg=Img1(idx_bg,:);
    bg_rgb = Img_bg/255; 
    bg_lab = rgb2lab(bg_rgb); 

end