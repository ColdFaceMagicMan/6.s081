#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"



int
main(int argc, char *argv[]){
    int left_fd[2];
    //int pid_0=fork();放在这里会导致父子进程拿到不同管道
    pipe(left_fd);
    int pid_0=fork();
    if(pid_0>0){
        close(left_fd[0]);
        for(int i=2;i<=35;++i){
            write(left_fd[1],&i,sizeof(int));
        }
        close(left_fd[1]);           
        wait(0);
        exit(0);
    }
    else{
        close(left_fd[1]);    
        int first_num;
        while(read(left_fd[0],&first_num,sizeof(int))){
          
            fprintf(2,"prime %d\n",first_num);            
            int right_fd[2];
            pipe(right_fd);  

            int read_num;
            while(read(left_fd[0],&read_num,sizeof(int))){
                if(read_num % first_num != 0){
                    write(right_fd[1],&read_num,sizeof(int));
                }

            }
            close(left_fd[0]);
            close(right_fd[1]);            

            int pid = fork();                
            if(pid == 0){
                left_fd[0]=dup(right_fd[0]);
                close(right_fd[0]);

            }
            else{
                close(right_fd[0]);                 
                wait(0);
                exit(0);//及时exit
            }
        }
        
    }
    exit(0);
}