#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/types.h> 
#include "compile.h"


struct compile_result compile(struct compile_parameter parameter)
{
    struct compile_result result;
    chdir(parameter.file_path);
    pid_t pid = fork();
    if(pid==0){
        struct rlimit lim;
        lim.rlim_cur=lim.rlim_max = 10;
        setrlimit(RLIMIT_CPU,&lim);
        freopen("compile_info.out","w",stderr);
        if(parameter.language == LANGUAGE_C){
            execlp("/usr/bin/gcc","gcc",parameter.file_name,"-o","main","-lm","-fmax-errors=5",NULL);
        }else if(parameter.language ==LANGUAGE_CPP){
            execlp("/usr/bin/g++","g++",parameter.file_name,"-o","main","-lm",NULL);
        }else if(parameter.language ==LANGUAGE_JAVA){

        }else{
            //语言类型错误
        }
        fclose(stderr);
        exit(0);
    }else{
        int status=0;
        waitpid(pid,&status,0);
        result.right = (!status);
        result.return_name = "main";
        result.return_info_name="compile_info.out";
    }
    return result;
}