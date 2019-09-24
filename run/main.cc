#include <stdio.h>
#include "run.h"
#include "compile.h"
int main()
{
    char file_path[50];
    char file_name[30];
    sprintf(file_path,"/home/tom/work/problem");
    sprintf(file_name,"main.c");

    struct compile_parameter com_par={file_path,file_name,1};
    struct compile_result com_res=compile(com_par);
    if(com_res.right==0){
        printf("编译错误\n");
        return 0;
    }
    sprintf(file_name,"%s",com_res.return_name);
    struct run_parameter parmeter={file_path,file_name,1,3,10,1000};
    struct run_result result = run(parmeter);
    printf("memory=%d\n",result.memory);
    printf("time=%d\n",result.time);
    printf("result=%d\n",result.result);
}