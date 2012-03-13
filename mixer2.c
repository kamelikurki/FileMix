#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>

#define BUFFRAMES 512

/**
Mixing software

**/

int main (int argc, char *argv[]) {

  /*Read the user parameters from a txt file*/
  if (argc != 3) {
    printf("Invalid number of arguments. Usage: \\mixer instructions_filename output_filename\n");
    exit(-1);
  }
  FILE *inputFile;
  char values[48];
  int i, j, fileCount, samplerate, framesWritten;
  sf_count_t maxFrames, count, maxCount;
  
  framesWritten = 0;
  j = 0;
  fileCount = 0;
  inputFile = fopen(argv[1], "r");
  
  // Count the number of input files
  while(fgets(values, 48, inputFile) != NULL) if(values[0] != '#')fileCount++;

  // Move the pointer back to the start of the file and do an error check
  if (fseek(inputFile, 0L, SEEK_SET) != 0) {
    printf("Error when moving the pointer back to the start of the file!");
    exit(-1);
  }

  float gain[fileCount], pan[fileCount], start[fileCount];
  char filename[fileCount][48];
  int skipFrames[fileCount];
  
  while(fgets(values, 48, inputFile) != NULL) {
    int parameters;
    if (values[0] != '#') {
      parameters = sscanf(values,"%s %f %f %f", filename[j], &start[j], &gain[j], &pan[j]);
      if(parameters != 4 ) {
	printf("Invalid number of parameters for %d. input file!\n", j+1);
	exit(-1);
      }
      if(start[j]<0 || pan[j]<0 || pan[j]>1 || gain[j]<0 || gain[j]>1) {
	printf("Please check that all the parameters for file %d. are in range(gain 0-1, pan 0-1, start 0-)! Exiting.\n", j+1);
	exit(-1);
      }
      j++;
    }
  }
  fclose(inputFile);
  /******************************************/



  /*Create soundfiles and their infos and allocate needed memory*/
  SNDFILE **inputs = (SNDFILE **) malloc(fileCount*sizeof(SNDFILE*));
  SF_INFO **info_inputs =(SF_INFO **) malloc(fileCount*sizeof(SF_INFO*));
  for(i=0;i<fileCount;i++) info_inputs[i] = (SF_INFO *) malloc(sizeof(SF_INFO));
  
  SNDFILE  *output;
  SF_INFO *info_output;
  info_output = (SF_INFO *) malloc(sizeof(SF_INFO));
  /**************************************************************/



  /*Open all input files, check sampling rates and formats and find maxFrames*/
  for(i=0;i<fileCount;i++) {
    int curFrames;
    // Open file for reading
    if (!(inputs[i] = sf_open(filename[i],SFM_READ,info_inputs[i]))) {
      printf("Error opening input file \"%s\"!\n",filename[i]); 
      exit(-1);
    }
    /* Set samplerate, how many empty frames are added to the beginning of the file...
       and total number of Frames needed for this file */
    if(i==0) {
      samplerate = (*info_inputs)->samplerate;
      skipFrames[i] = samplerate*start[i];
      curFrames = skipFrames[i]+((info_inputs[i])->frames);
      maxFrames = curFrames;
    } else {
      skipFrames[i] = samplerate*start[i];
      curFrames = skipFrames[i]+((info_inputs[i])->frames);
      // Check if samplerates match
      if(samplerate != info_inputs[i]->samplerate) {
	printf("Samplerates don't match!");
	exit(-1);
      }
      if(curFrames > maxFrames) maxFrames = curFrames;
    }
    // Check for format and precision
    if((SF_FORMAT_SUBMASK & (info_inputs[i])->format)>0x0008) {
      printf("File \"%s\" has invalid format or precision.\n", filename[i]);
      exit(-1);
    }
  }
  /****************************************************************************/



  /*Set up a file for output and open it for writing*/
  info_output->samplerate = samplerate;
  info_output->channels = 2;
  info_output->format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);
  info_output->seekable =(*info_inputs)->seekable;
  info_output->sections = (*info_inputs)->sections;
  info_output->frames = maxFrames;
  if(!(output = sf_open(argv[2],SFM_WRITE,info_output))){
    printf("error opening output file\n"); 
    exit(-1);
  }
  /**************************/



  /*set up buffers for holding the data*/
  float *outputBuffer;
  float **inputBuffers = (float **) malloc(fileCount*sizeof(float *));
  for(i=0;i<fileCount;i++) {
    if(info_inputs[i]->channels == 1) inputBuffers[i] = (float *) malloc(sizeof(float)*BUFFRAMES/2);
    else inputBuffers[i] = (float *) malloc(sizeof(float)*BUFFRAMES);
  }
  outputBuffer = (float *) malloc(sizeof(float)*BUFFRAMES);
  /************************************/



  /*Transfer the data from inputs to the output*/
  do {
    maxCount = 0;
    int x,y;
    for(y=0;y<fileCount;y++) {
      // write zeros to the beginning of the output file if starting time is not 0.
      if((framesWritten+BUFFRAMES)<skipFrames[y]) {
	maxCount = BUFFRAMES;
	for(x=0;x<512;x++) {
	  if(y==0)outputBuffer[x] = 0;
	  else outputBuffer[x] += 0;
	}
      }else{
	// read data to input buffers and move it to output buffer. Panning and gain are applied.
	// Mono file
	if(info_inputs[y]->channels == 1){
	  count = sf_readf_float(inputs[y],inputBuffers[y],BUFFRAMES/2);
	  if(count>maxCount) maxCount = count;
	  for(x=0;x<(BUFFRAMES/2);x++) {
	    if(y==0) {
	      outputBuffer[x*2] = pan[y]*gain[y]*(inputBuffers[y][x])/fileCount;
	      outputBuffer[x*2+1] = (1-pan[y])*gain[y]*(inputBuffers[y][x])/fileCount;
	    } else {
	      outputBuffer[x*2] += pan[y]*gain[y]*(inputBuffers[y][x])/fileCount;
	      outputBuffer[x*2+1] += (1-pan[y])*gain[y]*(inputBuffers[y][x])/fileCount;
	    }
	  }
	  //Stereo file
	} else {
	  count = sf_readf_float(inputs[y],inputBuffers[y],(BUFFRAMES/2));
	  if(count>maxCount) maxCount = count;
	  for(x=0;x<BUFFRAMES;x++) {
	    if(y==0) outputBuffer[x] = gain[y]*(inputBuffers[y][x])/fileCount;
	    else outputBuffer[x] += gain[y]*(inputBuffers[y][x])/fileCount;
	  }
	}
      }
    }
    sf_writef_float(output,outputBuffer,maxCount);
    framesWritten += maxCount;
  }while(maxCount);
  /*************************************/



  /*Close all files and free allocated memory*/
  for(i=0;i<fileCount;i++) {
    sf_close(inputs[i]);
    free(info_inputs[i]);
    free(inputBuffers[i]);
  }
  free(inputBuffers);
  free(info_inputs);
  free(inputs);
  sf_close(output);
  free(info_output);
  free(outputBuffer);
  /******************************************/
  return 0;
}
