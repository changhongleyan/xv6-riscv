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
    
    int pid = fork();
    if(pid == 0){
        struct data* p = (struct data*)shmget(key, sizeof(struct data), IPC_CREATE);
        if(p == 0){
            printf("shmget: error\n");
            exit(1);
        }
        
        char readbuf[100];
        printf("reading\n");
        while(p->length == 0){
            sleep(1);
        }
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
        printf("writing\n");
        memmove(p->data, buf, strlen(buf));
        p->length += strlen(buf);
        printf("write OK\n");
        wait(0);
    }
    
    exit(0);
}