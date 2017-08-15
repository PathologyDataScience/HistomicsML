function LAB = rgb2lab(RGB)
% Convert  RGB to lab(l,alpha,beta) to RGB
% Reinhard et al. Color Transfer between Images, IEEE Computer Graphics and Application,2001
%input:
%RGB - RGB sginals
%output:
%LAB - lab(l,alpha,beta)signals

% transformation between RGB and LMS cone space
Matrix1 = [0.3811 0.5783 0.0402;0.1967 0.7244 0.0782;0.0241 0.1288 0.8444];
LMS = [Matrix1 * RGB']';

% converting the data to logarithmic space
log_LMS = log10(LMS);

% transformation between LMS and lab
Matrix2 = diag([1/sqrt(3) 1/sqrt(6) 1/sqrt(2)]) * [1 1 1; 1 1 -2; 1 -1 0];
LAB = [Matrix2 * log_LMS']';

end

