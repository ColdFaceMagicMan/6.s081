#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

//将| xargs之前命令的输出添加为之后的参数
int
main(int argc, char *argv[]){
    char *new_argv[MAXARG];

    
    for(int i=1;i<argc;++i){//将xargs后参数读入新arg中
        new_argv[i-1]=argv[i];
    }

    char buf[512];
    char *buf_ptr=buf;


    new_argv[argc-1]=buf;
    while(read(0,buf_ptr,sizeof(char)) == sizeof(char)){//fd=0指向stdin
        if(*buf_ptr == '\n'){
            *buf_ptr = '\0';

            int pid=fork();
            if(pid == 0){
                exec(new_argv[0],new_argv);
                exit(0);
            }
            else{
                wait(0);                
                buf_ptr=buf;

            }
        }
        ++buf_ptr;
    }
    exit(0);
}
