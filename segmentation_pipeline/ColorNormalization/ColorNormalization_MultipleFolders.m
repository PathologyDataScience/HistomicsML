function ColorNormalization_MultipleFolders(Input_path,Output_path,target_mean,target_std,M)
%Performs normalization on all files(multiple folder with multiple files) 
% Inputs:
% Input_path - string containing path of input files.
% Output_path - string containing (existing) path for output files.
% target_mean - mapping parameter, output of the TargetParameters function
% target_std - mapping parameter, output of the TargetParameters function
% M - a 4x2 matrix containing the Fisher's linear discriminant function coefficients.
% M =[-0.154 0.035 0.549 -45.718; -0.057 -0.817 1.170 -49.887];
% DiscriminantF_BG =  M(1,1)*R + M(1,2)*G + M(1,3)*B + M(1,4);
% DiscriminantF_FG =  M(2,1)*R + M(2,2)*G + M(2,3)*B + M(2,4);

  % Reading multiple folders with multiple files
   
   subfolders=dir(Input_path);

   for k = 3:length(subfolders)  
       folder_name = subfolders(k).name;
       mkdir(Output_path,folder_name)
       
       InputFiles = dir(fullfile(Input_path,folder_name,'/*.tif*'));
       OutputFiles = dir(fullfile(Output_path,folder_name,'/*.tif*'));
       
       %convert to cell array of strings
       InputFiles = {InputFiles(:).name};
       OutputFiles = {OutputFiles(:).name};
       
       % Compare list of files, determine what is different
       file_list = setdiff(InputFiles, OutputFiles);
      
     for idx=1:length(file_list)
         original = imread([Input_path,'/',folder_name,'/',file_list{idx}]);
         [r,c,d] = size(original);
     
         % Segmenting foreground from background using discriminant functions
         o_fg = SegFG(original,M);
         o_bg = imcomplement(o_fg);

         % Applying Local_Reinhard's method   
          % Converting RGB signals to lab(l,alpha,beta) 
         [o_idx_fg,o_idx_bg,o_fg_lab,o_bg_lab] = PixelClass(original,o_fg,o_bg);
     
         % Mapping the color distribution of an image to that of the target image
         new_fg = transferI(o_fg_lab, target_mean, target_std);
         new_im=zeros(length(o_idx_fg)+length(o_idx_bg),3);
         new_im(o_idx_fg,:)=new_fg;
         new_im(o_idx_bg,:)=o_bg_lab;

         % Converting lab to RGB
         new_im = lab2rgb(new_im);  
         new_im = reshape(uint8(new_im*255), r,c,d);
         imwrite(new_im,[Output_path,'/',folder_name,'/','ColorNorm_',file_list{idx}],'tif','Compression','lzw'); 

     end  
     
   end
   
end
