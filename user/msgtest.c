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
    int pid = fork();
    if(pid){
        struct msgbuf sndbuf = {123, "hello world"};
        int msgid = msgget(key, IPC_CREATE);
        if(msgid == -1){
            exit(1);
        }
        msgsnd(msgid, &sndbuf, strlen(sndbuf.data));
        wait(0);
    } else {
        struct msgbuf rcvbuf;
        int msgid = msgget(key, IPC_CREATE);
        if(msgid == -1){
            exit(1);
        }
        int rcvlen = msgrcv(msgid, &rcvbuf, sizeof(rcvbuf.data), 123);
        printf("rcv\ntype: %d len: %d\nmsg: %s\n", rcvbuf.type, rcvlen, rcvbuf.data);
    }
    exit(0);
}