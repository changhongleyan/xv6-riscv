#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define PGSIZE  4096
#define MAXSIZE 1000
struct data{
    int length;
    char data[MAXSIZE];
};

int
main(int argc, char **argv)
{
    int key = 2;
    int semid = semget(0);
    int pid = fork();
    if(pid == 0){
        struct data* p = (struct data*)shmget(key, sizeof(struct data), IPC_CREATE);
        if(p == 0){
            printf("shmget: error\n");
            exit(1);
        }
        
        char readbuf[100];
        sem_p(semid);
        memmove(readbuf, p->data, p->length);
        p->length -= strlen(readbuf);
        printf("read: %s\n", readbuf);
    } else {
        struct data* p = (struct data*)shmget(key, sizeof(struct data), IPC_CREATE);
        if(p == 0){
            printf("shmget: error\n");
            exit(1);
        }

        char* buf = "Hello world!";
        p->length = 0;
        memmove(p->data, buf, strlen(buf));
        p->length += strlen(buf);
        sem_v(semid);
        wait(0);
    }
    semclose(semid);
    exit(0);
}