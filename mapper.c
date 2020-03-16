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
#include <string.h>
#include <semaphore.h>


#define MAXWORDSIZE 256
#define MAXLINESIZE 1024

typedef struct MapItem {
  // char word[MAXWORDSIZE];
  char word[MAXLINESIZE];
  int count;
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
int use  = 0;
int fill = 0;

sem_t empty;
sem_t full;
sem_t mutex;

List *boundedBuffer;
List *filePathList;

int insertNodeAtTail(List *, MapItem);
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

  boundedBuffer = (List *) malloc(sizeof(List));
  filePathList = (List *)malloc(sizeof(List));

  boundedBuffer->count = 0;
  boundedBuffer->head = NULL;
  boundedBuffer->tail = NULL;

  sem_init(&empty, 0, bBufferSize);
  sem_init(&full, 0, 0);
  sem_init(&mutex, 0, 1); 

  processCreator(argv[1]);

  printList(filePathList, 0);

  destroyList(filePathList);
  destroyList(boundedBuffer);

  printf("\nthis is the end of the main function: %d \n", getpid());

  return 0;
}

void processCreator(char *cmdFile) {

  printf("\nThis is the cmdFile in processCreator: %s \n",cmdFile);

  FILE *commandFilePtr = NULL;
  char *scannedWord = NULL;
  pid_t processID;
  int childPCount = 0;

  commandFilePtr = fopen(cmdFile, "r");

  if(commandFilePtr == NULL) {

    printf("The file %s could not be open in processCreator. Please try again!\n", cmdFile);
    exit(EXIT_FAILURE);

  }

  while(fscanf(commandFilePtr, "%ms", &scannedWord) != EOF) {
    printf("\nScanned word inside of processCreator: %s", scannedWord);

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
      printf("\nNumber of processes per parent: %d - %d \n", i ,getpid());
      printf("\nParent Waiting: %d \n", getpid());
      wait(NULL);
    }
  }

  printf("\nThis is the end of the processcreator: %d \n", getpid());

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
  int numberThreadsInArray = 5;
  pthread_t *workerThreadIDArray = NULL;
  pthread_t *tempWorkerThreadIDArray = NULL;
  pthread_t senderThreadID;

  workerThreadIDArray = (pthread_t *)malloc(sizeof(pthread_t)*5);

  threadDirPtr = opendir((*scannedWord));

  if(!threadDirPtr) {

    printf("\nThe directory %s could not be opened in threadCreator. Please try again!", (*scannedWord));
    exit(EXIT_FAILURE);

  }

  while((directoryStruct = readdir(threadDirPtr)) != NULL) {

    if((fileExtensionPtr = strrchr(directoryStruct->d_name,'.')) != NULL ) {
      
      if(strcmp(fileExtensionPtr, ".txt") == 0) {

        //Keeping track of all the thread IDs. Reallocating memory to the array if there are more threads
        //than expected
        if(numberThreadsCreated == numberThreadsInArray){
          tempWorkerThreadIDArray = (pthread_t *)realloc(workerThreadIDArray, sizeof(pthread_t)*5);
          
          if(tempWorkerThreadIDArray == NULL) {

            perror("realloc returned NULL");
            exit(1);

          } else {

            workerThreadIDArray = tempWorkerThreadIDArray;

          }

          numberThreadsInArray *= 5;
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

        printf("\nPATH: %s\n", filePathItem.word);
        pthread_create(&workerThreadIDArray[numberThreadsCreated], &workerThreadAttributes, mapItemCreator, filePathList->tail->item.word);

        free(filePath);

        numberThreadsCreated++;

      }

    }

  }

  pthread_attr_init(&senderThreadAttributes);
  pthread_create(&senderThreadID, &senderThreadAttributes, mapItemSender, NULL);

  for(int i = 0; i < numberThreadsCreated; i++){
    pthread_join(workerThreadIDArray[i], NULL);
    printf("\nJOIN THREAD: %d\n", numberThreadsCreated); 
  }

  pthread_join(senderThreadID, NULL);

  printf("\nThis is the # of nodes in dbll: %d\n", boundedBuffer->count);

  closedir(threadDirPtr);

  exit(0);
}


void * mapItemCreator(void *filePath) {

  FILE *filePtr = NULL;
  char *scannedWord = NULL;
  MapItem itemToSend;

  itemToSend.count = 1;
  filePtr = fopen((char *)filePath, "r");

  printf("\nPATH 2: %s\n", (char*)filePath);

  if(filePtr == NULL) {

    printf("\n The file %s could not be opened in mapItemCreator(). Please try again!\n", (char *)filePath);
    exit(EXIT_FAILURE);

  }
  
  while(fscanf(filePtr, "%ms", &scannedWord) != EOF) {

    strcpy(itemToSend.word, scannedWord);
    itemToSend.count = 1;

    sem_wait(&empty);
    sem_wait(&mutex);

    insertNodeAtTail(boundedBuffer , itemToSend);

    sem_post(&mutex);
    sem_post(&full);

    printf("\nProducer - WORD: %s \n", itemToSend.word);

  }

  pthread_exit(0);
}

void * mapItemSender(void * params) {
//printf("BOBO!\n");
  int message_queue_id;
  key_t messageKey;

// returns a key based on path and id(name of current file). The function returns the 
  //same key for all paths that point to the same file when called 
  //with the same id value. If ftok() is called with different id values 
  //or path points to different files on the same file system, it returns different keys.
  //key_t ftok(const char *path, int id);

  //We must create a thread that becomes the sender to the bounded buffer
  //we must implement the semaphores
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

  while(boundedBuffer->head->item.count != -1) {

    sem_wait(&full);
    sem_wait(&mutex);

    //used to send a message to the message queue specified by the msqid parameter. 
    //The *msgp parameter points to a user-defined buffer that must contain the 
    //following: A field of type long int that specifies the type of the message. 
    //A data part that contains the data bytes of the message.
    //int msgsnd(int msqid, void *msgp, size_t msgsz, int msgflg);

    printf("\n\tConsumer - WORD: %s \n", boundedBuffer->head->item.word);

    if(boundedBuffer->head->item.word != NULL && msgsnd(message_queue_id, &boundedBuffer->head->item, MAXWORDSIZE, 0) == -1)
      perror("msgsnd error in mapItemSender");
    //printf("\nRECEIVED: %s : %d", boundedBuffer->head->item.word, boundedBuffer->head->item.count);
    removeNodeAtHead(boundedBuffer);

    sem_post(&mutex);
    sem_post(&empty);

  }
  
  pthread_exit(0);
}

int insertNodeAtTail(List *listToGrow,MapItem itemToInsert) {

  // if(boundedBuffer->count == bBufferSize)
  //   return -1;

  Node *nextTailNode = malloc(sizeof(Node));

  nextTailNode->item = itemToInsert;

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
  fill++;

  //Testing
  if (fill == bBufferSize) {
    fill = 0;
  }

  //succesfully inserted node
  return 0;
}

void removeNodeAtHead(List * listToRemoveNode) {
  printf("\nRemove Func: %s\n", listToRemoveNode->head->item.word);

  Node *nodeToRemove = listToRemoveNode->head;

  if(nodeToRemove != NULL && nodeToRemove->item.word != NULL) {

    if(nodeToRemove->next == NULL) {

      listToRemoveNode->tail = NULL;
      listToRemoveNode->head = NULL;
      
      // free(nodeToRemove->item.word);
      free(nodeToRemove);

    } else {

      listToRemoveNode->head = nodeToRemove->next;
      nodeToRemove->next->prev = NULL;

      // free(nodeToRemove->item.word);
      free(nodeToRemove);

    }

  }

  listToRemoveNode->count --;
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

    free(nodeToDestroy->item.word);
    //free(nodeToDestroy->item);
    free(nodeToDestroy);

    nodeToDestroy = tempNode;

  }
}