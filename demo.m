clear all;
%% Demo of extraction of trace transform signatures 
A = imread('../test/Cam1_V1.pgm','pgm');        % Reading Image
A_smth = mat2gray(A);                   % Image pre-processing, illumination normalization.

%% Extraction of feature vectors
Code_Tfunct = [1];                      %Code of T functionals to be used, numbers comprised
                                        %between (1 to 7), which correspond to the files "FunctionalT1.c"
                                        
Code_Pfunct = [4];                      %Code of P functionals to be used, numbers between 1 and 3
                                        %use the classical P-functionals defined in "Apply_Pfunct",
                                        % from 4 onwards the Hermite functionals are employed.
                                        
angle_intrvl = 1;                       %3 degrees of angle interval between rotations
flag = 1;                               %Flag 0 (no sinogram orthonormalization) or 1 (sinogram orthonormalization)

% Main function that extracts the orthonormal signatures.
[Sinogram CircusF_a] = OrthTraceSign(A_smth,Code_Tfunct,Code_Pfunct, angle_intrvl,flag);
figure
imshow(mat2gray(Sinogram))
corr(CircusF_a)                         % Testing that the features extracted are actually orthonormal
figure
plot(CircusF_a)                         % Plotting the features extracted
