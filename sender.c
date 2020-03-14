#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MESSAGESIZE 128

struct message_s {
  long type;
  char content[MESSAGESIZE];
};

int main(void) {
  struct message_s message;
  int message_queue_id;
  key_t key;

  if ((key = ftok("sender.c", 1)) == -1) {
    perror("ftok");
    exit(1);
  }

  if ((message_queue_id = msgget(key, 0644 | IPC_CREAT)) == -1) {
    perror("msgget");
    exit(1);
  }

  message.type = 1;
  strcpy(message.content, "Studying Operating Systems Is Fun!\n");

  if(msgsnd(message_queue_id, &message, MESSAGESIZE, 0) == -1) {
    perror("Error in msgsnd");
  }

  return 0;
}
