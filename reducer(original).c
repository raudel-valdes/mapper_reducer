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

// message size ??
#define MAXWORDSIZE 256

typedef struct message{
    char word[MAXWORDSIZE];
    int count;
}Message;

typedef struct node{
    char* key;
    int count;
    struct node *next;
    struct node *prev;
}Node;

typedef struct list{
    struct node *head;
    struct node *tail;
}List;

void readMessages();
void insert(List *, Node *);
bool validate(List*, Message);
void add_message(List*, Message);
void printList(List *);
void destroy(List*);
Node* init_node(char *);
void destroy_node(Node *);
void initList(List *);


int main(int argc, char * argv[]){

   readMessages();

}

void readMessages() {

     int message_queue_id;
    Message msg;
    key_t key = ftok("mapper.c",1);

    List *lt = (List *) malloc(sizeof(List));

    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    if ((message_queue_id = msgget(key, 0444)) == -1) {
        perror("msgget");
        exit(1);
    }

    if (msgrcv(message_queue_id, &msg, MAXWORDSIZE, 0, 0) != -1)
        add_message(lt,msg);
    else {
        perror("Error in msgrcv"); 
        exit(1);
    }

    printf("\n This is the message recieved: %s : %d \n", msg.word, msg.count);

    // if (msgctl(message_queue_id, IPC_RMID, NULL) == -1) {
    //     perror("msgctl");
    //     exit(1);
    // }

    //printList(lt);

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
        if(strcmp(node->key, msg.word)==0){
            node->count++;
             exists=true;
        }
        node = node->next;
    }
    return exists;
}

void add_message(List* list, Message msg){
    
	if(!validate(list, msg)){
        Node *n = init_node(msg.word);
        insert(list, n);
	}
	// As a reference: if memory leaks, check for the node creation,
    // it may be the case that the insertion fails and the memory
    // allocated for the node is being not deallocated
    // If needed add a <else> clause to destroy the node
}

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

Node* init_node(char * key){
    Node *nd = (Node *) malloc(sizeof(Node));
    nd->key = strdup(key);
    nd->count = 1;
    return nd;
}

void destroy_node(Node *node) {
	free(node->key);
}

void initList(List *newlt){
    newlt->head=NULL;
    newlt->tail=NULL;
}