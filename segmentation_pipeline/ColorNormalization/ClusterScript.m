%script to perform color normalization on the cluster

close all; clear all; clc;

%parameters
M =[-0.154 0.035 0.549 -45.718; -0.057 -0.817 1.170 -49.887]; %foreground/background segmentation discriminant
TargetImage = '';
InputFolder = '/data2/Images/';
OutputFolder = '/data4/';
Mem = '512'; %free memory
Prefix = 'ColorNorm-';

%check if output folder exists
if(exist(OutputFolder, 'dir') ~= 7)
    mkdir(OutputFolder);
end

%get input file folders, each representing a slide, containing a set of tiles
Folders = dir(InputFolder);
Folders = {Folders(:).name};
Folders(strcmp(Folders, '.') | strcmp(Folders, '..')) = []; %clear '.' and '..'

%generate target image parameters
[Mean Std] = TargetParameters(TargetImage, M);

%iterate through folders, submitting one job for each
for i = 1:length(Folders)
    
    %check if output folder exists
    if(exist([OutputFolder Folders{i}], 'dir') ~= 7)
        mkdir([OutputFolder Folders{i}]);
    end
    
    %generate job string
    Job = sprintf('matlab -nojvm -nodesktop -nosplash -r "ColorNormalization_Folder(''%s'',''%s'',%s,%s,[%g %g %g]); pause(0.1); exit;" > %s\n',...
                    InputFolder, OutputFolder, Mean, Std, M, ['./' [Prefix num2str(i)] '.txt']);
    
    %submit job
    [status result] = SubmitPBSJob(Job, [Prefix num2str(i)], Mem);
    
end