#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>


// message size ??
#define MESSAGESIZE 128

typedef struct message{
    int count;
    char content[MESSAGESIZE];

}message;

typedef struct node{
    message msg;
    struct node *next;
    struct node *prev;
}node;

void init_node(){
    node *nd = (node *) malloc(sizeof(node));
}

typedef struct list{
    struct node *head;
    struct node *tail;
}list;

void initList(list *newlt){
	newlt->head=NULL;
	newlt->tail=NULL;
}

void insert(list *list, node *node){
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

void add_message(char * msg){

}

// Print out list
void printList(list *list){
    node *n = list->head;
    while(n!=NULL){
        printf("%s,%d\n", n->msg.content, n->msg.count);
        n = n->next;
    }
}

void destroy(list* list){
	node *node;
	while(list->head!=NULL){
		node = list->head;
		list->head=list->head->next;
		free(node);
	}
}

int main(int argc, char * argv[]){
    
    key_t key = ftok("mapper.c",1);
    int message_queue_id = msgget(key, 6445);
    message msg;

    list *lt = (list *) malloc(sizeof(list));
    
    // return -1 on failure and exit
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // return - on failure and exit
    if (message_queue_id == -1)
    {
        perror("msgget");
    }

    if (msgrcv(message_queue_id, &msg, MESSAGESIZE, 0, 0) != -1) {
        add(lt)
    }else
    {
        perror("msgrcv");
        exit(1);
    }

    // clear message, error out if -1
    if (msgctl(message_queue_id, IPC_RMID, NULL) == -1) {
    perror("msgctl");
    exit(1);
  }


}