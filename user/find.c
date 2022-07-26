#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


char*
fmtname(char *path)
{
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  return p;
}

void find(char* path,char* target_name){

  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }


  switch(st.type){
  case T_FILE:
    
    if(strcmp(fmtname(path),target_name)==0){
        printf("%s\n",path);                //使用fprintf会导致xargs无法读入
    }
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0 || strcmp(de.name,".") == 0 || strcmp(de.name,"..") == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf,target_name);
    }
    break;
  }
  close(fd);


}



int
main(int argc, char *argv[]){

    if(argc != 3){
        fprintf(2,"wrong arg");
        exit(1);
    }

    find(argv[1],argv[2]);

    exit(0);
}
