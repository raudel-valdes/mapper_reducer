#define _GNU_SOURCE

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

// message size
#define MESSAGESIZE 128

// semaphores
sem_t empty;
sem_t full;
sem_t mutex;


typedef struct message{
    long int type;
    char content[MESSAGESIZE];

}Message;

typedef struct node{
    char* key;
    int count;
    struct node *next;
    struct node *prev;
}Node;

Node* init_node(char * key){
    Node *nd = (Node *) malloc(sizeof(Node));
    nd->key = strdup(key);
    nd->count = 1;
    return nd;
}

void destroy_node(Node *node){
	free(node->key);
}

typedef struct list{
    struct node *head;
    struct node *tail;
}List;

void initList(List *newlt){
	newlt->head=NULL;
	newlt->tail=NULL;
}

void insert(List *list, Node *node){
    // If the list is empty point head and tail to the same element just created
    if(list->head==NULL){
        list->head=node;
        list->tail=node;
    }
    // If the list is not empty, insert at the end, update the tail reference
    else{
        list->tail->next=node;
        node->prev=list->tail;
        list->tail=node;
    }
}

bool validate(List* lt, Message msg){
    // search for the key, if found increment counter and return true
    Node *node = lt->head;
    bool exists=false;
    while(node!=NULL){
        if(strcmp(node->key, msg.content)==0){
            node->count++;
             exists=true;
        }
        node = node->next;
    }
    return exists;
}

void add_message(List* list, Message msg){
    
	if(!validate(list, msg)){
        Node *n = init_node(msg.content);
        insert(list, n);
	}
	// As a reference: if memory leaks, check for the node creation,
    // it may be the case that the insertion fails and the memory
    // allocated for the node is being not deallocated
    // If needed add a <else> clause to destroy the node
}

// Print out list
void printList(List *list){
    Node *n = list->head;
    while(n!=NULL){
        printf("%s,%d\n", n->key, n->count);
        n = n->next;
    }
}

void destroy(List* list){
	Node *node;
	while(list->head!=NULL){
		node = list->head;
		list->head=list->head->next;
        destroy_node(node);
		free(node);
	}
}

int main(int argc, char * argv[]){

    key_t key = ftok("mapper.c",1);
    int message_queue_id;
    Message msg;

    List *lt = (List *) malloc(sizeof(List));

    // return -1 on failure and exit
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    // connect to the queue
    if ((message_queue_id = msgget(key, 0644 )) == -1) {
    perror("msgget");
    exit(1);
  }
    // reducer can't stop til mapper sends the signal
    // while(1){
        // sem_wait(&full);
        // sem_wait(&mutex);
        while (msgrcv(message_queue_id, &msg, MESSAGESIZE, 0, 0) != -1) {
            printf("Message queue id: %d", message_queue_id);
            add_message(lt,msg);
        }
        // else
        //     {
        //         perror("Error in msgrcv"); 
        //         exit(1);
        //     }
    // }
     // destroy message queue
     // check using command ipcs -q
    if (msgctl(message_queue_id, IPC_RMID, NULL) == -1) {
    perror("msgctl");
    exit(1);
    }

    printList(lt);

    // sem_post(&mutex);
    // sem_post(&empty);
}