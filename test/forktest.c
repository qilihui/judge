#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
int main()
{
    pid_t pid = fork();
    if(pid==0){
        exit(0);
        return 0;
    }else if(pid >0){
        int status=0;
        int ret = wait(&status);
        printf("status = %d\n",status>>8);
        printf("ret = %d\n",ret);
    }else{
        printf("fork fail\n");
    }
}