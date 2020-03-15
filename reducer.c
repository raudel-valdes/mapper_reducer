#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

#define MESSAGESIZE 128

#define MAXWORDSIZE 256
#define MAXLINESIZE 1024

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

// Function declaration

int insertNodeAtTail(MapItem);
int removeNodeAtHead(List *);
void swapAdjNodes(List **, Node **, Node **);
void sortList(List *);
void printList(List *, int);
void destroyList(List *);

// Globals

List *messagesList;


int main(int argc, char * argv[]){
  int message_queue_id;
  key_t key;
  MapItem mRecieved;
  int msgrcvResponse = 0;

  if ((key = ftok("mapper.c",1)) == -1) {
    perror("ftok");
    exit(1);
  }

  if ((message_queue_id = msgget(key, 0444)) == -1) {
    perror("msgget 1");
    exit(1);
  }

  // msgrcvResponse = msgrcv(message_queue_id, &mRecieved, MAXWORDSIZE, 0, 0);
  
  // if(msgrcvResponse == -1) { 
  //   perror("msgrcv");
  //   exit(1);
  // }

  // printf("\nRECEIVED: %s : %d", mRecieved.word, mRecieved.count); 

  while ((msgrcvResponse = msgrcv(message_queue_id, &mRecieved, MAXWORDSIZE, 0, 0)) != -1){
    if(msgrcvResponse == -1) { 
      perror("msgrcv");
      exit(1);
    }
    printf("\nRECEIVED: %s : %d", mRecieved.word, mRecieved.count); 
  }
  
  

  //THIS IS USED TO ERASE THE WHOLE MESSAGE QUEUE. NOT A SINGLE MESSAGE!!
  // if (msgctl(message_queue_id, IPC_RMID, NULL) == -1) {
  //   perror("msgctl");
  //   exit(1);
  // }

}

int insertNodeAtTail(MapItem itemToInsert) {

  Node *nextTailNode = malloc(sizeof(Node));

  nextTailNode->item = itemToInsert;

  Node *currentHeadNode = messagesList->head;
  Node *currentTailNode = messagesList->tail;

  if (currentHeadNode == NULL) {

    nextTailNode->prev = NULL;
    nextTailNode->next = NULL;
    messagesList->head = nextTailNode;
    messagesList->tail = nextTailNode;

  } else {

    nextTailNode->prev = currentTailNode;
    nextTailNode->next = NULL;
    currentTailNode->next = nextTailNode;
    messagesList->tail = nextTailNode;
  }

  messagesList->count++;

  //succesfully inserted node
  return 0;
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
    free(nodeToDestroy);
    nodeToDestroy = tempNode;

  }
}


bool qualifyMessage(List* lt, MapItem msg){
    // search for the key, if found increment counter and return true
    Node *node = lt->head;
    bool exists=false;
    while(node!=NULL){
        if(strcmp(node->item.word, msg.word)==0){
            node->item.count++;
             exists=true;
        }
        node = node->next;
    }
    return exists;
}

void addMessages(List* list, MapItem msg){
    
	if(!qualifyMessage(list, msg)){
        insertNodeAtTail(msg);
	}
	// As a reference: if memory leaks, check for the node creation,
    // it may be the case that the insertion fails and the memory
    // allocated for the node is being not deallocated
    // If needed add a <else> clause to destroy the node
}
