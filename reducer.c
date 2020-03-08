#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MESSAGESIZE 128

typedef struct message{
    int count;
    char content[MESSAGESIZE];

}message;

int main(int argc, char * argv[]){
    
    key_t key = ftok("mapper.c",1);
    int message_queue_id = msgget(key, 6445);

    // return -1 on failure
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    if (message_queue_id == -1)
    {
        perror("msgget");
    }


    



}