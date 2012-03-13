FileMix

This program is made to mix one or more soundfiles into one file. 
It can also be used to transform mono files to stereo files or files from other formats to 
wav files.

Program takes two arguments from the command line: path for a .txt files that contains the mixing information and name of the desired output file
The .txt file should be of following format:
rows starting with hashtag(#) are comments
other rows should contain following parameters in following order:
name for input file, starting time for the sound in output file, mixing gain, panning

Panning is a value between 0 and 1(1 is left and 0 right, with stereo files this is ignored but some value needs to be given in the file)
Gain is a value between 0 and 1
starting time is given in seconds