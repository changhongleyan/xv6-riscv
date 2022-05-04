#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAXSIZE 1024
struct data{
    int length;
    char data[MAXSIZE];
};

int
main(int argc, char **argv)
{
    int key = 2;
    int synchro = semget(0);

    int pid = fork();
    if(pid){
        int shmid = shmget(key, sizeof(struct data), IPC_CREATE);
        struct data* p = (struct data*)shmva_get(shmid);
        char buf[] = "Hello world!";
        p->length = 0;

        memmove(p->data, buf, strlen(buf));
        p->length += strlen(buf);
        sem_v(synchro);

        wait(0);
    } else {
        int shmid = shmget(key, sizeof(struct data), IPC_CREATE);
        struct data* p = (struct data*)shmva_get(shmid);
        char readbuf[MAXSIZE];
        
        sem_p(synchro);
        int n = p->length;
        if(p->length > MAXSIZE){
            n = MAXSIZE;
        }
        memmove(readbuf, p->data, n);
        p->length -= n;
        
        printf("read: %s\n", readbuf);
    }

    semclose(synchro);
    exit(0);
}