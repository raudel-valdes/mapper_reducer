//This is for the mythreads.h 
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h> 
#include <sys/msg.h>


//This header file came from the CECS-420 class website
#include "mythreads.h"


void processCreator(char *[]);
void threadCreator(char **);
void *mapItemCreator(void *);

#define MAXWORDSIZE 256
#define MAXLINESIZE 1024

typedef struct mapItemStruct {
  char word[MAXWORDSIZE];
  int count;
} mapItemStruct;

int max;
int items;
int *buffer;

int main(int argc, char *argv[]) {

  if(argc != 3) {
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

    printf("The file %s could not be open. Please try again!\n", arguments[1]);
    exit(EXIT_FAILURE);

  }

  while(fscanf(commandFilePtr, "%ms", &scannedWord) == 1 && processID == 0) {

    processID = fork();

    if (processID < 0) {
      printf("Fork Failed\n");
      exit(-1);
    }

    if(processID == 0)
      threadCreator(&scannedWord);
    else wait(NULL);

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
        pthread_create(&threadID, &threadAttributes, mapItemCreator, directoryStruct->d_name);
        pthread_join(threadID, NULL);

      }

    }

  }

  closedir(threadDirPtr);

}


void *mapItemCreator(void *fileName) {
  FILE *filePtr = NULL;
  char *scannedWord = NULL;
  mapItemStruct *mapItem = malloc(sizeof(mapItemStruct));  
  mapItem->count = 1;
  int message_queue_id;
  key_t messageKey;

  filePtr = fopen(fileName, "r");

  // returns a key based on path and id(name of current file). The function returns the 
  //same key for all paths that point to the same file when called 
  //with the same id value. If ftok() is called with different id values 
  //or path points to different files on the same file system, it returns different keys.
  //key_t ftok(const char *path, int id);
  if ((messageKey = ftok("mapper.c", 1)) == -1) {
    perror("ftok");
    exit(1);
  }

  //returns the System V message queue identifier associated with the value of the key argument.
  //It may be used either to obtain the identifier of a previously created message
  //queue (when msgflg is zero and key does not have the value IPC_PRIVATE), or to create a new set.
  //A new message queue is created if key has the value IPC_PRIVATE or key isn't IPC_PRIVATE, 
  //no message queue with the given key key exists, and IPC_CREAT is specified in msgflg.
  //int msgget(key_t key, int msgflg);
  if ((message_queue_id = msgget(messageKey, 0644 | IPC_CREAT)) == -1) {
    perror("msgget");
    exit(1);
  }

  while(fscanf(filePtr, "%ms", &scannedWord) == 1) {

    strcpy(mapItem->word, scannedWord);

    //used to send a message to the message queue specified by the msqid parameter. 
    //The *msgp parameter points to a user-defined buffer that must contain the 
    //following: A field of type long int that specifies the type of the message. 
    //A data part that contains the data bytes of the message.
    //int msgsnd(int msqid, void *msgp, size_t msgsz, int msgflg);
    if(msgsnd(message_queue_id, &mapItem, MAXWORDSIZE, 0) == -1)
      perror("Error in msgsnd");

  }

  pthread_exit(0);
}