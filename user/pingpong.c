#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]){

    if(argc!=1){
        fprintf(2,"input error");
        exit(1);
    }

    int fd_1[2];//其实一个管道也能完成
    int fd_2[2];
    char content;
    if(pipe(fd_1)!=0){
        exit(1);
    }
    if(pipe(fd_2)!=0){
        exit(1);
    }

    if(fork()==0){
        close(fd_2[0]);
        close(fd_1[1]);
        read(fd_1[0],&content,1);
        close(fd_1[0]);
        fprintf(2,"%d: received ping\n",getpid());
        write(fd_2[1],"pong",1);
        close(fd_2[1]);
    }
    else{
        close(fd_1[0]);
        close(fd_2[1]);
        write(fd_1[1],"ping",1);
        close(fd_1[1]);
        read(fd_2[0],&content,1);
        fprintf(2,"%d: received pong\n",getpid());//要加空格，不然过不了grade
        close(fd_2[0]);
        wait(0);
    }

    exit(0);

}