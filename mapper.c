#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>

void processCreator(char *[]);
void threadCreator(char **);
void *mapItemCreator(void *);

int main(int argc, char *argv[]) {

  if(argc != 2) {
    printf("Please include the following things at execution time: \n");
    printf("\t 1) commandFile. \n");
    printf("\t 2) bufferSize \n");
    exit(EXIT_FAILURE);
  }

  processCreator(argv);


  return 0;
}

void processCreator(char *arguments[]) {
  FILE *commandFilePtr = NULL;
  char *scannedWord = NULL;
  pid_t processID;

  commandFilePtr = fopen(arguments[1], "r");

  if(commandFilePtr == NULL) {

    printf("The file %s could not be open. Please try again!", arguments[1]);
    exit(EXIT_FAILURE);

  }

  while(fscanf(commandFilePtr, "%ms", &scannedWord) != EOF) {

    processID = fork();

    if (processID < 0) {
      printf("Fork Failed\n");
      exit(-1);
    }

    threadCreator(&scannedWord);

  }

}

void threadCreator(char **scannedWord) {
  DIR *threadDirPtr;
  struct dirent *directoryStruct;
  char *fileExtensionPtr = NULL;
  pthread_t threadID;
  pthread_attr_t threadAttributes;

  threadDirPtr = opendir((*scannedWord));

  if(!threadDirPtr) {

    printf("The file %s could not be open. Please try again!", (*scannedWord));
    exit(EXIT_FAILURE);

  }

  while((directoryStruct = readdir(threadDirPtr)) != NULL) {

    if((fileExtensionPtr = strrchr(directoryStruct->d_name,'.')) != NULL ) {
      
      if(strcmp(fileExtensionPtr, ".txt") == 0) {

        pthread_attr_init(&threadAttributes);
        pthread_create(&threadID, &threadAttributes, mapItemCreator, &directoryStruct->d_name);
        pthread_join(threadID, NULL);

      }

    }

  }

  closedir(threadDirPtr);

}


void *mapItemCreator(void *filePath) {
  printf("Hello There \n");

  pthread_exit(0);
}