#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define SEGSIZE 1024
#define M 1024 * 1024
#define EPOCH 16 * 1024

#define READ 0
#define WRITE 1
int
ipc_pipe()
{
    int fd[2];
    pipe(fd);
    int tick0 = uptime();
    int pid = fork();
    if (pid){
        char writebuf[SEGSIZE];
        close(fd[READ]);
        for(int i = 0; i < EPOCH; ++i){
            memset(writebuf, i, sizeof(writebuf));
            write(fd[WRITE], writebuf, sizeof(writebuf));
        }
        close(fd[WRITE]);
        wait(0);
    }
    else
    {
        char readbuf[SEGSIZE];
        close(fd[WRITE]);
        for(int i = 0; i < EPOCH; ++i){
            read(fd[WRITE], readbuf, sizeof(readbuf));
            memset(readbuf, 0, sizeof(readbuf));
        }
        close(fd[WRITE]);
    }
    if(pid)
        return uptime() - tick0;
    else
        exit(0);
}

int
ipc_fifo()
{
    char* path = "fifo";
    mkfifo(path, O_CREATE);
    int tick0 = uptime();

    int pid = fork();
    if (pid){
        char writebuf[SEGSIZE];
        int fdw = open(path, O_WRFIFO);
        for(int i = 0; i < EPOCH; ++i){
            memset(writebuf, i, sizeof(writebuf));
            write(fdw, writebuf, sizeof(writebuf));
        }
        close(fdw);
        wait(0);
    }
    else
    {
        char readbuf[SEGSIZE];
        int fdr = open(path, O_RDFIFO);
        for(int i = 0; i < EPOCH; ++i){
            read(fdr, readbuf, sizeof(readbuf));
            memset(readbuf, 0, sizeof(readbuf));
        }
        close(fdr);
    }
    if(pid)
        return uptime() - tick0;
    else
        exit(0);
}

struct msgbuf{
    int type;
    char data[SEGSIZE];
};
int
ipc_msg()
{
    int key = 2;
    int semid = semget(0);
    int tick0 = uptime();

    int pid = fork();
    if(pid){
        int msgid = msgget(key, IPC_CREATE);
        struct msgbuf sndbuf = {123};
        for(int i = 0; i < EPOCH; ++i){
            memset(sndbuf.data, i, sizeof(sndbuf.data));
            msgsnd(msgid, &sndbuf, sizeof(sndbuf.data));
            sem_v(semid);
        }
        wait(0);
    } else {
        int msgid = msgget(key, IPC_CREATE);
        struct msgbuf rcvbuf;
        for(int i = 0; i < EPOCH; ++i){
            sem_p(semid);
            msgrcv(msgid, &rcvbuf, sizeof(rcvbuf.data), 123);
            memset(rcvbuf.data, 0, sizeof(rcvbuf.data));
        }
    }

    semclose(semid);
    if(pid)
        return uptime() - tick0;
    else
        exit(0);
}

struct shmbuf{
    int length;
    int size;
    char data[16 * SEGSIZE];
};
int
ipc_shm()
{
    int key = 2;
    int mutex = semget(1);
    int synchro = semget(0);
    int tick0 = uptime();

    int pid = fork();
    if(pid){
        struct shmbuf* p = (struct shmbuf*)shmget(key, sizeof(struct shmbuf), IPC_CREATE);
        p->size = sizeof(p->data);
        p->length = 0;
        sem_v(synchro);
        for(int i = 0; i < EPOCH;){
            int n;
            sem_p(mutex);
            if(p->size == p->length){
                sem_v(mutex);
                continue;
            }
            n = p->size - p->length;
            memset(p->data + p->length, i, n);
            p->length = p->size;
            sem_v(mutex);
            i += n / SEGSIZE;
            
        }
        wait(0);
    } else {
        struct shmbuf* p = (struct shmbuf*)shmget(key, sizeof(struct shmbuf), IPC_CREATE);
        sem_p(synchro);
        for(int i = 0; i < EPOCH;){
            int n;
            sem_p(mutex);
            if(p->length == 0){
                sem_v(mutex);
                continue;
            }
            n = p->length;
            memset(p->data, 0, n);
            p->length = 0;
            sem_v(mutex);
            i += n / SEGSIZE;
        }
    }
    semclose(mutex);
    semclose(synchro);
    if(pid)
        return uptime() - tick0;
    else
        exit(0);
}

int
main(int argc, char **argv)
{
    printf("pipe: %d\n", ipc_pipe());
    printf("fifo: %d\n", ipc_fifo());
    printf("msg: %d\n", ipc_msg());
    printf("shm: %d\n", ipc_shm());
    exit(0);
}