function SegmentationProcessTiles(Slide, Desired, T, Folder)
%Uses openslide library to process tiles in slide.
%Paths must be set to OpenSlide, ColorDeconvolution and BoundaryValidator.
%inputs:
%Slide - string containing slide path and filename with extension.
%Desired - scalar, desired magnification.
%T - scalar, tilesize, natural number.
%Folder - path to outputs

%start timer
tstart

%parameters
M = [-0.154 0.035 0.549 -45.718; -0.057 -0.817 1.170 -49.887]; %color normalization - discriminate tissue/background.
TargetImage = '/home/lcoop22/ImageAnalysis/ColorNormalization/GBM_target1.tiff';

%add paths
addpath /home/lcoop22/ImageAnalysis/OpenSlide/
addpath /home/lcoop22/ImageAnalysis/ColorDeconvolution/
addpath /home/lcoop22/ImageAnalysis/ColorNormalization/
addpath /home/lcoop22/ImageAnalysis/BoundaryValidator/

%check if slide can be opened
Valid = openslide_can_open(Slide);

%slide is a valid file
if(Valid)
    
    %generate schedule for desired magnification
    [Level, Scale, Tout, Factor, X, Y, dX, dY] = ...
        TilingSchedule(Slide, Desired, T);
    
    %calculate normalization parameters
    [tMean tStd] = TargetParameters(imread(TargetImage), M);
    
    %update console
    fprintf('Processing image %s, at magnification %d. Resizing factor %d.\n',...
        Slide, Desired, Factor);
    
    %process each tile
    for i = 1:length(X)
        
        %start timer
        tStart = tic;
        
        %generate coordinates and filename
        Xstr = sprintf('%06.0f', dX(i));
        Ystr = sprintf('%06.0f', dY(i));
        Slashes = strfind(Slide, '/');
        Dots = strfind(Slide, '.');
        OutputFilename = [Folder Slide(Slashes(end)+1:Dots(end)-1) '.' Xstr '.' Ystr '.mat'];
        
        %check if file exists
        if(~exist(OutputFilename, 'file'))
            
            %read in tile
            I = openslide_read_regions(Slide, Level, X(i), Y(i), Tout, Tout);
                 
            %update console
            fprintf('\tProcessing tile %06.0f.%06.0f, %d of %d ', dX(i), dY(i), i, length(X));
            
            %resize if necessary
            if(Factor ~= 1)
                I = imresize(I{1}, Factor, 'bilinear');
            else
                I = I{1};
            end
            
            %normalize color
            RGB = ColorNormalization_Tile(I, tMean, tStd, M);
            
            %foreground/background segmentation
            Foreground = Foreground(RGB);
            
            %proceed if nuclei are present
            if(sum(Foreground(:)) > 0)
                
                %individual cell segmentation
                [Label, bX, bY] = Individual(Foreground);
                
                %if labeled objects exist
                if(~isempty(bX))
                    
                    %feature extraction
                    [Features, Names, cX, cY] = FeatureExtraction(Label, RGB);
                    
                    %add scan and analysis magnifications to features
                    ScanMag = repmat(Desired/Scale, [size(Features,1) 1]);
                    AnalysisMag = repmat(Desired, [size(Features, 1) 1]);
                    Features = [ScanMag AnalysisMag Features];
                    Names = ['ScanMag', 'AnalysisMag', Names];
                    
                    %continue processing if objects were located
                    if(~isempty(Features))
                        
                        %place boundaries, centroids in global frame,
                        %correct for resizing
                        cX = cX + dX(i);
                        cY= cY + dY(i);
                        bX = cellfun(@(x) x + dX(i), bX, 'UniformOutput', false);
                        bY = cellfun(@(x) x + dY(i), bY, 'UniformOutput', false);
                        
                        %embed boundaries and output tile
                        Mask = bwperim(Label > 0, 4);
                        R = RGB(:,:,1); R(Mask) = 0;
                        G = RGB(:,:,2); G(Mask) = 255;
                        B = RGB(:,:,3); B(Mask) = 0;
                        I = cat(3,R,G,B);
                        
                        %stop timer
                        Elapsed = toc(tStart);
                        
                        %write results to disk
                        save([Folder Slide(Slashes(end)+1:Dots(end)-1) '.' Xstr '.' Ystr '.mat'],...
                            'Foreground', 'Label', 'bX', 'bY', 'Features', 'Names', 'cX', 'cY', 'Elapsed');
                        imwrite(I, [Folder Slide(Slashes(end)+1:Dots(end)-1) '.' Xstr '.' Ystr '.jpg']);
                        
                        %merge colinear points on boundaries
                        for k = 1:length(bX)
                            [bX{k}, bY{k}] = MergeColinear(bX{k}, bY{k});
                        end
                        
                        %generate database txt file
                        SegmentationReport([Folder Slide(Slashes(end)+1:Dots(end)-1) '.seg.' Xstr '.' Ystr '.txt'],...
                            Slide(Slashes(end)+1:Dots(end)-1), cX, cY, Features, Names, bX, bY);
                        
                    end
                end
            end
            
            %update console
            fprintf('%g seconds.\n', toc(tStart));
            
        end
    end
    
else
    
    %display error
    error(['Cannot open slide ' Slide]);
    
end