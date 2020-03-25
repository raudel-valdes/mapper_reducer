#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h> 
#include <sys/msg.h>
#include <string.h>
#include <semaphore.h>


#define MAXWORDSIZE 256
#define MAXLINESIZE 1024

typedef struct MapItem {

  int count;
  char word[MAXWORDSIZE];

} MapItem;

typedef struct Node{
  MapItem item;
  struct Node *next;
  struct Node *prev;
} Node;

typedef struct List {
  Node *head;
  Node *tail;
  int count;
} List;

int bBufferSize = 0;

sem_t empty;
sem_t full;
sem_t mutex;

List *boundedBuffer;
List *filePathList;

void insertNodeAtTail(List *, MapItem);
void removeNodeAtHead(List *);
void sortList(List *);
void swapAdjNodes(List **, Node **, Node **);
void printList(List *, int);
void destroyList(List *);
void processCreator(char *);
void threadCreator(char **);
void *mapItemCreator(void*);
void *mapItemSender(void*);

int main(int argc, char *argv[]) {

  if(argc != 3) {
    printf("Please include the following things at execution time: \n");
    printf("\t 1) commandFile. \n");
    printf("\t 2) bufferSize \n");
    exit(EXIT_FAILURE);
  }

  bBufferSize = atoi(argv[2]);

  boundedBuffer = (List *)malloc(sizeof(List));
  filePathList = (List *)malloc(sizeof(List));

  boundedBuffer->count = 0;
  boundedBuffer->head = NULL;
  boundedBuffer->tail = NULL;

  sem_init(&empty, 0, bBufferSize);
  sem_init(&full, 0, 0);
  sem_init(&mutex, 0, 1); 

  processCreator(argv[1]);

  destroyList(filePathList);
  // destroyList(boundedBuffer);

  free(boundedBuffer);

//***********************************************
//FOR SOME REASON THE PROGRAM WON'T TERMINATE WITHOUT THIS
// SEEMS LIKE IT LOOSED THE FILEPATH BEFORE TERMINATING
//***********************************************

  // 24 bytes reported by valgrind 
  //free(filePathList);

  printf("\nthis is the end of the main function: %d \n", getpid());

  return 0;
}

void processCreator(char *cmdFile) {

  FILE *commandFilePtr = NULL;
  char *scannedWord = NULL;
  pid_t processID;
  int childPCount = 0;

  commandFilePtr = fopen(cmdFile, "r");

  if(commandFilePtr == NULL) {
    printf("could not open file %s\n", cmdFile);
    exit(1);
  }

  while(fscanf(commandFilePtr, "%ms", &scannedWord) != EOF) {

    processID = fork();

    printf("\nprocessID inside of processCreator: %d \n", getpid());

    if (processID < 0) {
      printf("Fork Failed\n");
      exit(-1);
    }

    if(processID == 0) {
      threadCreator(&scannedWord);
    }

    childPCount++;

  }

  if(processID != 0) {
    for(int i = 0; i < childPCount; i++) {
      printf("\nParent Waiting: %d \n", getpid());
      wait(NULL);
    }
  }

  // added by Oz
  fclose(commandFilePtr);

  printf("\n\n \tMain Process returned!!: %d \n\n", getpid());

}

void threadCreator(char **scannedWord) {

  //directory and filepath related variables
  DIR *threadDirPtr;
  struct dirent *directoryStruct;
  char *fileExtensionPtr = NULL;
  char *filePath;
  char *lastCharInDir = NULL;
  MapItem filePathItem;

  //thread related variables
  pthread_attr_t workerThreadAttributes;
  pthread_attr_t senderThreadAttributes;
  int numberThreadsCreated = 0;
  int sizeOfThreadArray = 5;
  pthread_t *workerThreadIDArray = NULL;
  void * retvals[1];
  pthread_t senderThreadID;
  void * senderThreadRetval[1];

  MapItem lastWord;

  workerThreadIDArray = (pthread_t *)malloc(sizeof(pthread_t)*5);
  
  threadDirPtr = opendir((*scannedWord));

  if(!threadDirPtr) {
    perror("Could not open directory \n");
    exit(1);
  }

  while((directoryStruct = readdir(threadDirPtr)) != NULL) {

    if((fileExtensionPtr = strrchr(directoryStruct->d_name,'.')) != NULL ) {
      
      if(strcmp(fileExtensionPtr, ".txt") == 0) {

        //Keeping track of all the thread IDs. Reallocating memory to the array if there are more threads
        //than expected
        if(numberThreadsCreated == sizeOfThreadArray) {
          
          workerThreadIDArray = (pthread_t *)realloc(workerThreadIDArray, sizeof(pthread_t)*5);

          if(workerThreadIDArray == NULL) {

            perror("realloc returned NULL");
            exit(1);

          }

          sizeOfThreadArray *= 5;
        }

        workerThreadIDArray[numberThreadsCreated] = numberThreadsCreated;

        filePath = (char *)malloc(sizeof(char)*MAXLINESIZE);
        strcat(filePath, *scannedWord);

        //Does a check to see if the filepath contains the / or not
        //before it is concatinated to the name of the file.
        //if it doesn't then it will include it
        lastCharInDir = strrchr(*scannedWord, '/');
        if(strcmp(lastCharInDir, "/") != 0) {
          strcat(filePath, "/");
        }

        strcat(filePath, directoryStruct->d_name);
        strcpy(filePathItem.word, filePath);
        insertNodeAtTail(filePathList, filePathItem);

        pthread_attr_init(&workerThreadAttributes);

        if (pthread_create(&workerThreadIDArray[numberThreadsCreated], &workerThreadAttributes, mapItemCreator, filePathList->tail->item.word)) {
          perror("Cannot create thread\n");
          exit(1);
        }

        numberThreadsCreated++;

      }

    }

  }

  pthread_attr_init(&senderThreadAttributes);

  pthread_create(&senderThreadID, &senderThreadAttributes, mapItemSender, NULL);

  for(int i = 0; i < numberThreadsCreated; i++){

    if (pthread_join(workerThreadIDArray[i], &retvals[i]) != 0) {
      perror("\nCannot join thread\n");
      exit(1);
    }
  }

    sem_wait(&empty);
    sem_wait(&mutex);

    strcpy(lastWord.word, "END OPERATIONS!");
    lastWord.count = -1;

    insertNodeAtTail(boundedBuffer, lastWord);

    sem_post(&mutex);
    sem_post(&full);


  if (pthread_join(senderThreadID, &senderThreadRetval[0]) != 0) {
    perror("\nCannot join thread\n");
    exit(1);
  }
  
  closedir(threadDirPtr);
  free(workerThreadIDArray);
  free(filePath);

  exit(0);
}


void * mapItemCreator(void *filePath) {

  FILE *filePtr = NULL;
  char *scannedWord = NULL;
  MapItem itemToSend;

  itemToSend.count = 1;
  filePtr = fopen((char *)filePath, "r");

  if(filePtr == NULL) {
    printf("\n The file %s could not be opened in mapItemCreator(). Please try again!\n", (char *)filePath);
    exit(1);
  }
  
  while(fscanf(filePtr, "%ms", &scannedWord) != EOF) {

    strcpy(itemToSend.word, scannedWord);
    itemToSend.count = 1;

    //printf("\n\n//////////////////PRODUCER/////////////////\n");

    sem_wait(&empty);
    sem_wait(&mutex);

    insertNodeAtTail(boundedBuffer, itemToSend);
    printf("\nProducer - WORD: %s, PID: %d\n", itemToSend.word, getpid());

    sem_post(&mutex);
    sem_post(&full);

  }
  
  printf("\n\n\tTerminating Producer Thread!! \n\n");
  free(scannedWord);
  
  fclose(filePtr);

  pthread_exit(0);
}

void * mapItemSender(void * params) {

  printf("\n\tConsumer Started!\n");

  int message_queue_id;
  key_t messageKey;

  if ((messageKey = ftok("mapper.c", 1)) == -1) {
    perror("ftok");
    exit(1);
  }

  if ((message_queue_id = msgget(messageKey, 0644 | IPC_CREAT)) == -1) {
    perror("msgget");
    exit(1);
  }
  int counter = 0;
  
  while(boundedBuffer->head != NULL && boundedBuffer->head->item.count != -1) {

  counter ++;

    sem_wait(&full);
    sem_wait(&mutex);

    if(boundedBuffer->head->item.word != NULL && msgsnd(message_queue_id, &boundedBuffer->head->item, MAXWORDSIZE, 0) == -1)
      perror("msgsnd error in mapItemSender");

    removeNodeAtHead(boundedBuffer);
    printf("\nCONSUMER: WORD: %s, PID: %d is extracted.\n", boundedBuffer->head->item.word, getpid());

    sem_post(&mutex);
    sem_post(&empty);

  }

  if(boundedBuffer->head->item.word != NULL && msgsnd(message_queue_id, &boundedBuffer->head->item, MAXWORDSIZE, 0) == -1)
    perror("msgsnd error in mapItemSender2");

  printf("\n\n\tTerminating Consumer Thread!! \n\n");

  pthread_exit(0);
}

void insertNodeAtTail(List *listToGrow,MapItem itemToInsert) {

  Node *nextTailNode = malloc(sizeof(Node));

  strcpy(nextTailNode->item.word, itemToInsert.word);
  nextTailNode->item.count = itemToInsert.count;

  Node *currentHeadNode = boundedBuffer->head;
  Node *currentTailNode = boundedBuffer->tail;

  if (currentHeadNode == NULL) {

    nextTailNode->prev = NULL;
    nextTailNode->next = NULL;

    listToGrow->head = nextTailNode;
    listToGrow->tail = nextTailNode;

  } else {

    nextTailNode->prev = currentTailNode;
    nextTailNode->next = NULL;
    currentTailNode->next = nextTailNode;
    listToGrow->tail = nextTailNode;

  }

  listToGrow->count++;
}

void removeNodeAtHead(List * listToRemoveNode) {
  printf("\nREMOVENODEATHEAD: %s\n", listToRemoveNode->head->item.word);

  Node *nodeToRemove = listToRemoveNode->head;

  if(nodeToRemove != NULL && nodeToRemove->item.word != NULL) {

    if(nodeToRemove->next == NULL) {

      // listToRemoveNode->tail = NULL;
      // listToRemoveNode->head = NULL;

      // free(nodeToRemove);

    } else {

      listToRemoveNode->head = nodeToRemove->next;
      nodeToRemove->next->prev = NULL;

     free(nodeToRemove);
    }

    listToRemoveNode->count --;
  }
}

void sortList(List *unsortedList) {

  Node *marker = NULL;
  Node *markerPrev = NULL;
  Node *compareNode = NULL;
  Node *originalSwap = NULL;

  markerPrev = unsortedList->head;
  marker = unsortedList->head->next;  

  while(marker != NULL && markerPrev != NULL) {

    if (strcmp(markerPrev->item.word, marker->item.word) < 0) {

      marker = marker->next;
      markerPrev = markerPrev->next;

    } else { 

      swapAdjNodes(&unsortedList, &markerPrev, &marker);

      originalSwap = marker;
      marker = markerPrev->next;
      compareNode = originalSwap->prev;
      
      while(compareNode != NULL && originalSwap != NULL && (strcmp(compareNode->item.word, originalSwap->item.word) > 0)) {
       
        swapAdjNodes(&unsortedList, &compareNode, &originalSwap);
        compareNode = originalSwap->prev;
      }

      if (marker != NULL)
        markerPrev = marker->prev;

    }
  }

}

void swapAdjNodes(List **unsortedList, Node **nodeOne, Node **nodeTwo) {

  Node *tempNode = NULL;
  tempNode = (*nodeOne)->prev;

  if(tempNode != NULL) {

    tempNode->next = (*nodeTwo);
    (*nodeTwo)->prev = tempNode;
    (*nodeOne)->next = (*nodeTwo)->next;
    (*nodeTwo)->next = (*nodeOne);

  } else {

    (*nodeTwo)->prev = tempNode;
    (*nodeOne)->next = (*nodeTwo)->next;
    (*nodeTwo)->next = (*nodeOne);
    (*unsortedList)->head = (*nodeTwo);

  }

  tempNode = (*nodeOne)->next;

  if(tempNode != NULL) {

    tempNode->prev = (*nodeOne);
    (*nodeOne)->prev = (*nodeTwo);

  } else {

    (*nodeOne)->prev = (*nodeTwo);
    (*unsortedList)->tail = (*nodeOne);

  }

}

void printList(List *list, int reverse) {

  Node *currentNode = NULL;

  if (!reverse) {
    currentNode = list->head;
    
    while (currentNode != NULL) {

      printf("%s:%d\n", currentNode->item.word, currentNode->item.count);
      currentNode = currentNode->next;

    }

  } else {
    
    currentNode = list->tail;
    
    while (currentNode != NULL) {

      printf("%s,%d\n", currentNode->item.word, currentNode->item.count);
      currentNode = currentNode->prev;

    }

  }

}

void destroyList(List *listToDestroy) {
  Node *nodeToDestroy = NULL;
  Node *tempNode = NULL;

  nodeToDestroy = listToDestroy->head;
  while(nodeToDestroy != NULL) {
    tempNode = nodeToDestroy->next;

    free(nodeToDestroy);
    nodeToDestroy = tempNode;

  }
}