function RGB = lab2rgb(LAB)
% Convert lab(l,alpha,beta) to RGB
% Reinhard et al. Color Transfer between Images, IEEE Computer Graphics and Application,2001
%input:
%LAB - lab(l,alpha,beta)signals
%output:
%RGB - RGB sginals

% from lab to LMS
Matrix1 =  [1 1 1; 1 1 -1; 1 -2 0] * diag([sqrt(3)/3 sqrt(6)/6 sqrt(2)/2]);
log_LMS = [Matrix1 * LAB']';

% conver back from log space to linear space
LMS = 10.^log_LMS;
LMS(find(LMS==-Inf)) = 0;
LMS(isnan(LMS)) = 0;

% from linear LMS to RGB
Matrix2 = [4.4687 -3.5887 0.1196;-1.2197 2.3831 -0.1626;0.0585 -0.2611 1.2057];
RGB = [Matrix2 * LMS']';

end
