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

  if ((message_queue_id = msgget(key, 0444)) == -1) {
    perror("msgget");
    exit(1);
  }

  if (msgrcv(message_queue_id, &message, MESSAGESIZE, 0, 0) == -1) {
    perror("msgrcv");
    exit(1);
  }

  printf("%s", message.content);

  if (msgctl(message_queue_id, IPC_RMID, NULL) == -1) {
    perror("msgctl");
    exit(1);
  }

  return 0;
}
