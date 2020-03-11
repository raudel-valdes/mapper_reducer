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


#define MAXWORDSIZE 256
#define MAXLINESIZE 1024

int bBufferSize = 0;
//Might be necessary
// int max;
// int items;
// int *buffer;

typedef struct MapItem {
  char word[MAXWORDSIZE];
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

int insertNodeAtTail(List *, MapItem);
int removeNodeAtHead(List *);
void sortList(List *);
void swapAdjNodes(List **, Node **, Node **);
void printList(List *, int);
void destroyList(List *);
void processCreator(char *);
void threadCreator(char **);
void *mapItemCreator(void *);

int main(int argc, char *argv[]) {

  if(argc != 3) {
    printf("Please include the following things at execution time: \n");
    printf("\t 1) commandFile. \n");
    printf("\t 2) bufferSize \n");
    exit(EXIT_FAILURE);
  }

  bBufferSize = atoi(argv[2]);
  processCreator(argv[1]);


  printf("\nthis is the end of the main function: %d \n", getpid());
  return 0;
}

void processCreator(char *cmdFile) {

  printf("This is the cmdFile: %s",cmdFile);
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

    printf("Child ID: %d - Parent ID: %d \n", getpid(), getppid());
    printf("End of the processCreator while loop\n");
  }

  if(processID != 0) {
    for(int i = 0; i < childPCount; i++) {
      // printf("\nNumber of processes per parent: %d - %d \n", childPCount ,getpid());
      // printf("Parent Waiting: %d \n", getpid());
      wait(NULL);
    }
  }

  printf("\nThis is the end of the processcreator: %d \n", getpid());

}

void threadCreator(char **scannedWord) {

  DIR *threadDirPtr;
  struct dirent *directoryStruct;
  char *fileExtensionPtr = NULL;
  pthread_t threadID;
  pthread_attr_t threadAttributes;
  char threadMapItemPath[MAXLINESIZE] = {};

  strcat(threadMapItemPath, *scannedWord);
  strcat(threadMapItemPath, "/");
  threadDirPtr = opendir((*scannedWord));

  if(!threadDirPtr) {

    printf("\nThe directory %s could not be opened in threadCreator. Please try again!", (*scannedWord));
    exit(EXIT_FAILURE);

  }

  while((directoryStruct = readdir(threadDirPtr)) != NULL) {

    if((fileExtensionPtr = strrchr(directoryStruct->d_name,'.')) != NULL ) {
      
      if(strcmp(fileExtensionPtr, ".txt") == 0) {

        strcat(threadMapItemPath, directoryStruct->d_name);

        pthread_attr_init(&threadAttributes);
        pthread_create(&threadID, &threadAttributes, mapItemCreator, (void *)threadMapItemPath);
        pthread_join(threadID, NULL);

      }

    }

  }

  closedir(threadDirPtr);
  exit(0);
}


void *mapItemCreator(void *filePath) {

  FILE *filePtr = NULL;
  char *scannedWord = NULL;
  int message_queue_id;
  key_t messageKey;

  MapItem mapItem;
  mapItem.count = 1;

  filePtr = fopen(filePath, "r");

  if(filePtr == NULL) {

    printf("\n The file %s could not be opened in mapItemCreator(). Please try again!\n", (char *)filePath);
    exit(EXIT_FAILURE);

  }

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

  while(fscanf(filePtr, "%ms", &scannedWord) != EOF) {

    strcpy(mapItem.word, scannedWord);

    //used to send a message to the message queue specified by the msqid parameter. 
    //The *msgp parameter points to a user-defined buffer that must contain the 
    //following: A field of type long int that specifies the type of the message. 
    //A data part that contains the data bytes of the message.
    //int msgsnd(int msqid, void *msgp, size_t msgsz, int msgflg);

    if(msgsnd(message_queue_id, &mapItem, MAXWORDSIZE, 0) == -1)
      perror("Error in msgsnd");

    printf("\nMESSAGE SENT: %s", mapItem.word);
  }

    printf("\n\n\n\t This Thread Finished Sending Messages!!! %d \n\n\n\n", getpid());

  pthread_exit(0);
}

int insertNodeAtTail(List *firstFileList, MapItem itemToInsert) {

  if(firstFileList->count == bBufferSize)
    //buffer is full. Can't insert
    return -1;

  Node *nextTailNode = malloc(sizeof(Node));

  nextTailNode->item = itemToInsert;

  Node *currentHeadNode = firstFileList->head;
  Node *currentTailNode = firstFileList->tail;

  if (currentHeadNode == NULL) {

    nextTailNode->prev = NULL;
    nextTailNode->next = NULL;

    firstFileList->head = nextTailNode;
    firstFileList->tail = nextTailNode;

  } else {

    nextTailNode->prev = currentTailNode;
    nextTailNode->next = NULL;
    currentTailNode->next = nextTailNode;
    firstFileList->tail = nextTailNode;

  }

  firstFileList->count++;

  //succesfully inserted node
  return 0;
}

int removeNodeAtHead(List * listToRemoveNode) {

  Node *nodeToRemove = listToRemoveNode->head;

  if(listToRemoveNode->count == 0)
    //cannot remove node. There aren't any
    return nodeToRemove = -1;

  if(nodeToRemove == NULL)
    //cannot remove node. Node not fund
    return nodeToRemove = -2;


  listToRemoveNode->head = nodeToRemove->next;
  nodeToRemove->next->prev = NULL;
  nodeToRemove->next = NULL;
  nodeToRemove->prev = NULL;

  free(nodeToRemove->item.word);
  //free(nodeToRemove->item);
  free(nodeToRemove);

  //Success removing the node
  return 0;

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