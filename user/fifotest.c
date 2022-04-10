#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "user.h"

int
main(int argc, char **argv)
{      
    char* path = "fifo";
    mkfifo(path, O_CREATE);

    int pid = fork();
    if(pid == 0){
        char* buf = "Hello world!";
        int fdw = open(path, O_WRFIFO);
        write(fdw, buf, strlen(buf));
        close(fdw);
    }
    else{
        char buf[100];
        int fdr = open(path, O_RDFIFO);
        int n = read(fdr, buf, 12);
        close(fdr);
        printf("read %d: %s\n", n, buf);
    }
    exit(0);
}