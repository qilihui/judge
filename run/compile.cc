#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "compile.h"


struct compile_result compile(struct compile_parameter parameter)
{
    // Parameter parameter;
    struct compile_result result;
    chdir(parameter.file_path);
    // parameter.file_path = argv[1];
    // parameter.file_name = argv[2];
    // parameter.language = atoi(argv[3]);
    // printf("%s\n%s\n%d\n",parameter.file_path,parameter.file_name,parameter.language);
    pid_t pid = fork();
    if(pid==0){
        struct rlimit lim;
        lim.rlim_cur=lim.rlim_max = 10;
        setrlimit(RLIMIT_CPU,&lim);
        // lim.rlim_cur=lim.rlim_max = 
        if(parameter.language == LANGUAGE_C){
            // freopen("/home/tom/work/out.out","w",stdout);
            freopen("compile_info.out","w",stderr);
            const char *c_compile[] = {"gcc",(const char*)parameter.file_name,"-o","main",NULL};
            // sprintf(c_compile[1],"%s/%s",parameter.file_path,parameter.file_name);
            execvp("gcc",(char *const*)c_compile);
        }else if(parameter.language ==LANGUAGE_CPP){
            char *cpp_compile;
            sprintf(cpp_compile,"g++ %s/%s -o main",parameter.file_path,parameter.file_name);
            execlp("g++",cpp_compile,NULL);
        }else if(parameter.language ==LANGUAGE_JAVA){

        }else{
            //语言类型错误
        }
        fclose(stderr);
        exit(0);
        // fclose(stdout);
    }else{
        // printf("%d\n",pid);
        int status=0;
        waitpid(pid,&status,0);
        // printf("%d\n",status);
        result.right = (!status);
        // result.return_path = (char *)work_path;
        result.return_name = (char *)"main";
    }
    // printf("%d\n%s\n%s\n",result.right,result.return_path,result.return_name);
    return result;
}