#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

struct msgbuf{
    int type;
    char data[128];
};

int 
main(int argc, char **argv)
{
    int key = 2;
    int semid = semget(0);
    int pid = fork();
    if(pid){
        struct msgbuf sndbuf = {123, "hello world"};
        int msgid = msgget(key, IPC_CREATE);
        if(msgid == -1){
            exit(1);
        }
        msgsnd(msgid, &sndbuf, strlen(sndbuf.data));
        sem_v(semid);
        wait(0);
    } else {
        struct msgbuf rcvbuf;
        int msgid = msgget(key, IPC_CREATE);
        if(msgid == -1){
            exit(1);
        }
        sem_p(semid);
        msgrcv(msgid, &rcvbuf, sizeof(rcvbuf.data), 123);
        printf("rcv\ntype: %d\nmsg: %s\n", rcvbuf.type, rcvbuf.data);
    }
    semclose(semid);
    exit(0);
}