#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ 0
#define WRITE 1

int
main(int argc, char *argv[])
{
    int pipe_parent[2];
    pipe(pipe_parent);

    int pid = fork();
    if(pid > 0){   
        close(pipe_parent[READ]);
        for(int i = 2; i <= 35; ++i){
            write(pipe_parent[WRITE], &i, sizeof(int));
        }
        close(pipe_parent[WRITE]);
        wait(0);
        exit(0);
    }
    else{   
        close(pipe_parent[WRITE]);
        int min_prime;
        while(read(pipe_parent[READ], &min_prime, sizeof(int))){
            printf("prime %d\n", min_prime);
            
            int pipe_child[2];
            pipe(pipe_child);
            int num;
            while(read(pipe_parent[READ], &num, sizeof(int))){    
                if(num % min_prime){
                    write(pipe_child[WRITE], &num, sizeof(int));
                }
            }
            close(pipe_child[WRITE]);

            int pid2 = fork();
            if(pid2 == 0){
                pipe_parent[READ] = dup(pipe_child[READ]);
                close(pipe_child[READ]);
                printf("pid%d: READ%d\n", getpid(), pipe_parent[READ]);
            }
            else{
                close(pipe_child[READ]);
                wait(0);
                exit(0);
            }
        }
        exit(0);
    }
}