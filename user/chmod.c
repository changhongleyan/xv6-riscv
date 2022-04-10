#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]){
    if(argc <= 2){
        printf("too few arguments\n");
        exit(0);
    }
    chmod(argv[1], atoi(argv[2]));
    exit(0);
}