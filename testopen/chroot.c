#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
int main()
{
    char a[100];
    if(!getcwd(a,sizeof(a))){
        printf("getcwd 失败\n");
    }
    printf("%s\n",a);
    if(chroot("./")){
        printf("执行chroot 失败 %s\n",strerror(errno));
    }
    if(!getcwd(a,sizeof(a))){
        printf("getcwd 失败\n");
    }
    printf("%s\n",a);
    if(chdir("./dir")){
        printf("chdir 失败\n");
    }
    if(!getcwd(a,sizeof(a))){
        printf("getcwd 失败\n");
    }
    printf("%s\n",a);
    if(access("main",F_OK)){
        printf("文件不存在\n");
    }
    if(execl("main","main",NULL)){
        printf("exec 失败  %s\n",strerror(errno));
    }
}