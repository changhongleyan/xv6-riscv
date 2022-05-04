#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define SEGSIZE 1024
#define EPOCH 256 * 1024

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
    } else {
        char readbuf[SEGSIZE];
        close(fd[WRITE]);
        for(int i = 0; i < EPOCH; ++i){
            read(fd[READ], readbuf, sizeof(readbuf));
            memset(readbuf, 0, sizeof(readbuf));
        }
        close(fd[READ]);
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
    } else {
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
ipc_msg(int key)
{
    int msgid;
    int synchro = semget(0);
    int tick0 = uptime();

    int pid = fork();
    if(pid){
        msgid = msgget(key, IPC_CREATE);
        struct msgbuf sndbuf = {123};
        for(int i = 0; i < EPOCH; ++i){
            memset(sndbuf.data, i, sizeof(sndbuf.data));
            msgsnd(msgid, &sndbuf, sizeof(sndbuf.data));
            sem_v(synchro);
        }
        wait(0);
    } else {
        msgid = msgget(key, IPC_CREATE);
        struct msgbuf rcvbuf;
        for(int i = 0; i < EPOCH; ++i){
            sem_p(synchro);
            msgrcv(msgid, &rcvbuf, sizeof(rcvbuf.data), 123);
            memset(rcvbuf.data, 0, sizeof(rcvbuf.data));
        }
    }
    semclose(synchro);
    msgclose(msgid);
    if(pid)
        return uptime() - tick0;
    else
        exit(0);
}

struct shmbuf{
    int length;
    int size;
    char data[8 * SEGSIZE];
};
int
ipc_shm(int key)
{
    int shmid;
    int mutex = semget(1);
    int synchro = semget(0);
    int tick0 = uptime();

    int pid = fork();
    if(pid){
        shmid = shmget(key, sizeof(struct shmbuf), IPC_CREATE);
        struct shmbuf* p = (struct shmbuf*)shmva_get(shmid);
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
        shmid = shmget(key, sizeof(struct shmbuf), IPC_CREATE);;
        struct shmbuf* p = (struct shmbuf*)shmva_get(shmid);
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
    shmclose(shmid);
    if(pid)
        return uptime() - tick0;
    else
        exit(0);
}

int
main(int argc, char **argv)
{
    for(int i = 0; i < 20; ++i){
        printf("%d ", i);
        //printf("pipe: %d ", ipc_pipe());
        //printf("fifo: %d ", ipc_fifo());
        //printf("msg: %d ", ipc_msg(i));
        printf("shm: %d ", ipc_shm(i));
        printf("\n");
    }
    exit(0);
}