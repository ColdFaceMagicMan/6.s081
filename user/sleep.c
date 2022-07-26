#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int
main(int argc, char *argv[]){

    if(argc!=2){
        fprintf(2,"usage: sleep seconds\n");
        exit(0);
    }

    sleep(atoi(argv[1]));
    exit(0);
}