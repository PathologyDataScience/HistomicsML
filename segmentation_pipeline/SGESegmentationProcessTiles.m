%script to perform nuclei segmentation and feature extraction on the cluster

close all; clear all; clc;

%parameters
Desired = 20; %objective magnification
T = 4096; %tilesize
InputFolder = '/home/lcoop22/Images/GBM/';
OutputFolder = '/home/lcoop22/Test/';
Mem = '512'; %free memory
Prefix = 'LGG.';

%check if output folder exists
if(exist(OutputFolder, 'dir') ~= 7)
    mkdir(OutputFolder);
end

%get list of .svs files
Files = dir([InputFolder '*.svs']);
Files = {Files.name};
Slides = cellfun(@(x)x(1:end-4), Files, 'UniformOutput', false);
Dots = strfind(Slides, '.');
Slides = cellfun(@(x,y)x(1:y(end)-1), Slides, Dots, 'UniformOutput', false);
Files = cellfun(@(x)[InputFolder x], Files, 'UniformOutput', false);

%iterate through slides, submitting one job per slide
for i = 1:length(Files)

   %check if output folder exists
   if(exist([OutputFolder Slides{i}], 'dir') ~= 7)
       mkdir([OutputFolder Slides{i}]);
   end
   
   %generate job string
   Job = sprintf('matlab -nojvm -nodesktop -nosplash -logfile "%s" -r "addpath /home/lcoop22/ImageAnalysis/NewSegmentation/Pipeline/; SegmentationProcessTiles(''%s'', %g, %g, ''%s''); exit;"',...
       [OutputFolder Prefix num2str(i) '.txt'],...
       Files{i}, Desired, T,...
       [OutputFolder Slides{i} '/']);
   
   %submit job
   [status, result] = SubmitTorqueJob(Job, [Prefix num2str(i)], Mem);
   
   %update console
   fprintf('job %d, folder: %s, status: %s.', i, Files{i}, result);
    
end