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
    char* path = "mmap";
    int key = 2;
    
    int pid = fork();
    if(pid == 0){
        int fd = open(path, O_CREATE | O_RDWR);
        int shmid = shmget(key);
        if(shmid < 0){
            printf("shmget: error\n");
        }
        struct data* p = (struct data*)mmap(0, sizeof(struct data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0, shmid);
        close(fd);

        char* buf = "Hello world!";
        p->length = 0;
        printf("writing\n");
        memmove(p->data, buf, strlen(buf));
        p->length += strlen(buf);
        printf("write OK\n");
    }else{
        int fd = open(path, O_CREATE | O_RDWR);
        int shmid = shmget(key);
        if(shmid < 0){
            printf("shmget: error\n");
        }
        struct data* p = (struct data*)mmap(0, sizeof(struct data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0, shmid);
        close(fd);
        
        char readbuf[100];
        while(p->length == 0){
            sleep(1);
        }
        printf("reading\n");
        memmove(readbuf, p->data, p->length);
        p->length -= strlen(readbuf);
        printf("read: %s\n", readbuf);
    }
    
    exit(0);
}