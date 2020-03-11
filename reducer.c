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
#define MAXLINESIZE 1024

typedef struct message {
  char word[MAXWORDSIZE];
  int count;
} message;

int main () {
  int message_queue_id;
  key_t key;
  message mRecieved;
  int msgrcvResponse = 0;

  if ((key = ftok("mapper.c",1)) == -1) {
    perror("ftok");
    exit(1);
  }

  if ((message_queue_id = msgget(key, 0444)) == -1) {
      perror("msgget");
      exit(1);
  }

  while (msgrcv(message_queue_id, &mRecieved, MAXWORDSIZE, 0, 0) != -1)
    printf("\nRECEIVED: %s : %d", mRecieved.word, mRecieved.count); 
  
  if(msgrcvResponse == -1) { 
    perror("msgrcv");
    exit(1);
  }

  //THIS IS USED TO ERASE THE WHOLE MESSAGE QUEUE. NOT A SINGLE MESSAGE!!
  // if (msgctl(message_queue_id, IPC_RMID, NULL) == -1) {
  //   perror("msgctl");
  //   exit(1);
  // }

}