function [f, Names, CentroidX, CentroidY] = FeatureExtraction(L, color_img, K, Fs, delta)
%Calculate features from color image and segmentation label image.
%inputs:
%L - T x T label image.
%color_img - T x T x 3 color image.
%K - number of points for boundary resampling to calculate fourier descriptors (recommended 128).
%Fs - number of frequency bins for calculating FSDs.
%delta - scalar, used to dilate nuclei and define cytoplasm region.

if(nargin < 3)
	K = 128; Fs = 6; delta = 8;
end

num = max(L(:));

smallG = double(color_img(:,:,2))./(double(sum(color_img, 3))+eps);
[Gx, Gy] = gradient(smallG);
diffG = sqrt(Gx.*Gx+Gy.*Gy);
BW_canny = edge(smallG,'canny');

statsI = regionprops(L, smallG, 'Area','Perimeter','Eccentricity',...
                        'MajorAxisLength','MinorAxisLength','Extent',...
                        'Solidity','PixelIdxList','Centroid','BoundingBox');

%morphometry features                    
fArea = cat(1,statsI.Area);
fPerimeter = cat(1,statsI.Perimeter);
fEccentricity = cat(1,statsI.Eccentricity);
fCircularity = 4*pi * fArea./ (fPerimeter.^2);
fMajorAxisLength = cat(1,statsI.MajorAxisLength);
fMinorAxisLength = cat(1,statsI.MinorAxisLength);
fExtent = cat(1,statsI.Extent);
fSolidity = cat(1,statsI.Solidity);
fMorph = [fArea, fPerimeter, fEccentricity, fCircularity,fMajorAxisLength,...
            fMinorAxisLength, fExtent, fSolidity];
MorphNames = {'Area', 'Perimeter', 'Eccentricity', 'Circularity',...
                'MajorAxisLength', 'MinorAxisLength', 'Extent', 'Solidity'};

%centroids
Centroids = cat(1,statsI.Centroid);
CentroidX = Centroids(:,1);
CentroidY = Centroids(:,2);

%generate object pixel lists for nuclear and cytoplasmic regions
Nuclei = cell(1,num);
Cytoplasms = cell(1,num);
Bounds = cell(1,num);
disk = strel('disk', delta, 0); %create round structuring element
for i = 1:num
    Nuclei{i} = statsI(i).PixelIdxList; %capture nuclear region from regionprops on 'L'
    bounds = GetBounds(statsI(i).BoundingBox, delta, size(L,1), size(L,2)); %get bounds of dilated nucleus
    Nucleus = L(bounds(3):bounds(4), bounds(1):bounds(2)) == i; %grab nucleus mask
    Trace = bwboundaries(Nucleus,8, 'noholes');
    Bounds{i} = Trace{1};
    Mask = L(bounds(3):bounds(4), bounds(1):bounds(2)) > 0; %get mask for all nuclei in neighborhood
    cytoplasm = xor(Mask, imdilate(Nucleus, disk)); %remove nucleus region from cytoplasm+nucleus mask
    Cytoplasms{i} = PixIndex(cytoplasm, bounds, size(L,1), size(L,2)); %get list of cytoplasm pixels
end

%calculate FSDs
Interval = round(log2space(0, log2(K)-1, Fs+1)); %six fourier descriptors, spaced evenly over the interval 1:K/2 (spectrum is symmetric)
FSDNames = cellfun(@(x,y) [x num2str(y)], repmat({'FSD'}, [1,Fs]), num2cell([1:Fs]), 'UniformOutput', false);
FSDGroup = zeros(num, Fs);
for i = 1:num
    FSDGroup(i,:) = FSDs(Bounds{i}(:,1), Bounds{i}(:,2), K, Interval);
end

%deconvolve color image to calculate nuclear, cytoplasmic texture features
Deconvolved = ColorDeconvolution(color_img, [0.650 0.072 0; 0.704 0.990 0; 0 0 0], [true true false]); %H&E 2 color matrix
Hematoxylin = Deconvolved(:,:,1);
Hematoxylin(Hematoxylin > 255) = 255;
Eosin = Deconvolved(:,:,2);
Eosin(Eosin > 255) = 255;
clear Deconvolved;

%convert to double format
Hematoxylin = double(Hematoxylin);
Eosin = double(Eosin);

%calculate hematoxlyin features, capture feature names
[HematoxylinIntensityGroup, IntensityNames] = IntensityFeatureGroup(Hematoxylin, Nuclei);
[HematoxylinTextureGroup, TextureNames] = TextureFeatureGroup(Hematoxylin, Nuclei);
[HematoxylinGradientGroup, GradientNames] = GradientFeatureGroup(Hematoxylin, Nuclei);

%calculate eosin features
EosinIntensityGroup = IntensityFeatureGroup(Eosin, Cytoplasms);
EosinTextureGroup = TextureFeatureGroup(Eosin, Cytoplasms);
EosinGradientGroup = GradientFeatureGroup(Eosin, Cytoplasms);

%combine
fDeconvolved = [HematoxylinIntensityGroup HematoxylinTextureGroup HematoxylinGradientGroup...
        EosinIntensityGroup EosinTextureGroup EosinGradientGroup];

%check features
if(max(L(:)) ~= size(fMorph,1) || max(L(:)) ~= size(fDeconvolved,1))
    error('No. of rows in feature set is not equal to number of labeled objects!\n');
end
if size(fMorph,1)~=size(fDeconvolved,1)
    error('No. of rows in the old feature set is not equal to that of the new feature set!\n');
end

%concatenate features
f = [fMorph FSDGroup fDeconvolved];
   
%create 'Names' output
Names = {IntensityNames{:} TextureNames{:} GradientNames{:}};
NuclearNames = cellfun(@(x)strcat('Hematoxlyin', x), Names, 'UniformOutput', false);
CytoplasmNames = cellfun(@(x)strcat('Cytoplasm', x), Names, 'UniformOutput', false);
Names = {MorphNames{:} FSDNames{:} NuclearNames{:} CytoplasmNames{:}};

end


function low_s = lowB(s)
S = fft(s);

order = 30;
cutoff_freq = 30;
c = fir1(order, cutoff_freq/length(s)/2);
C = fft(c,length(s));
low_s = ifft(C'.*S);
low_s(end) = low_s(1);
end

function [f names] = IntensityFeatureGroup(I, ObjectPixelList)
f = zeros(length(ObjectPixelList), 4); %initialize 'f'

for i = 1:length(ObjectPixelList) %calculate for each object
    pixOfInterest = I(ObjectPixelList{i});
    f(i,1) = double(mean(pixOfInterest));
    f(i,2) = f(i,1) - double(median(pixOfInterest));
    f(i,3) = max(pixOfInterest);
    f(i,4) = min(pixOfInterest);
    f(i,5) = std(double(pixOfInterest));
end

names = {'MeanIntensity', 'MeanMedianDifferenceIntensity', 'MaxIntensity', 'MinIntensity', 'StdIntensity'};
end

function [f names]= TextureFeatureGroup(I, ObjectPixelList)
f = zeros(length(ObjectPixelList), 4);
for i = 1:length(ObjectPixelList)
    pixOfInterest = I(ObjectPixelList{i});
    [counts] = imhist(uint8(pixOfInterest));
    prob = counts/sum(counts);
    
    f(i,1) = entropy(uint8(pixOfInterest));
    f(i,2) = sum(prob.^2);
    f(i,3) = skewness( double(pixOfInterest) );
    f(i,4) = kurtosis( double(pixOfInterest) );
end

names = {'Entropy', 'Energy', 'Skewness', 'Kurtosis'};

end

function [f names] = GradientFeatureGroup(I, ObjectPixelList)
[Gx, Gy] = gradient(double(I));
diffG = sqrt(Gx.*Gx+Gy.*Gy);
BW_canny = edge(I,'canny');
seSmall = strel('disk', 2);

f = zeros(length(ObjectPixelList), 8);
for i = 1:length(ObjectPixelList)
    pixOfInterest = diffG(ObjectPixelList{i});
    fMeanGradMag = mean(pixOfInterest);
    fStdGradMag = std(pixOfInterest);
    [counts,binLocation] = imhist(uint8(pixOfInterest));
    prob = counts/sum(counts);
    
    fEntropyGradMag = entropy(uint8(pixOfInterest));
    fEnergyGradMag = sum(prob.^2);
    fSkewnessGradMag = skewness( double(pixOfInterest) );
    fKurtosisGradMag = kurtosis( double(pixOfInterest) );
    
    bw_canny = BW_canny(ObjectPixelList{i});
    fSumCanny = sum(bw_canny(:));
    
    fMeanCanny = fSumCanny / length(pixOfInterest);
    
    f(i,:) = [fMeanGradMag, fStdGradMag, fEntropyGradMag, fEnergyGradMag,...
        fSkewnessGradMag,fKurtosisGradMag, fSumCanny, fMeanCanny];
end

names = {'MeanGradMag', 'StdGradMag', 'EntropyGradMag', 'EnergyGradMag',...
    'SkewnessGradMag', 'KurtosisGradMag', 'SumCanny', 'MeanCanny'};
end

function bounds = GetBounds(bbox, delta, M, N)
%get bounds of object in global label image
bounds(1) = max(1,floor(bbox(1) - delta));
bounds(2) = min(N, ceil(bbox(1) + bbox(3) + delta));
bounds(3) = max(1,floor(bbox(2) - delta));
bounds(4) = min(M, ceil(bbox(2) + bbox(4) + delta));
end

function idx = PixIndex(Binary, bounds, M, N)
%get global linear indices of object extracted from tile
[i j] = find(Binary);
i = i + bounds(3) - 1;
j = j + bounds(1) - 1;
idx = sub2ind([M N], i, j);
end