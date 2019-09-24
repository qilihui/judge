#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "run.h"

struct run_result run(struct run_parameter parameter)
{
    struct run_result result;
    //跳转到工作目录
    chdir((const char *)parameter.file_path);
    system("pwd");
    result.memory=0;
    result.result=0;
    result.time=0;
    const char *user_out="user.out";
    for(int i=1;i<=parameter.case_num;i++){
        pid_t pid = fork();
        if(pid==0){
            struct rlimit lim;
            lim.rlim_cur=lim.rlim_max=parameter.time/1000+1;
            setrlimit(RLIMIT_CPU,&lim);
            lim.rlim_cur=lim.rlim_max=parameter.memory*__MB__*2;
            setrlimit(RLIMIT_AS,&lim);
            lim.rlim_cur=lim.rlim_max=__MB__;
            setrlimit(RLIMIT_FSIZE,&lim);
            lim.rlim_cur=lim.rlim_max=1;
            setrlimit(RLIMIT_NPROC,&lim);
            char input_name[20];
            char file_name[20];
            sprintf(input_name,"%d.in",i);
            sprintf(file_name,"./%s",parameter.file_name);
            freopen((const char*)input_name,"r",stdin);
            freopen(user_out,"w",stdout);
            freopen("err.out","w",stderr);
            // printf("开始执行\n");
            execl((const char *)file_name,(const char *)file_name,NULL);
            // system("pwd");
            // system("./main");
        }else{
            struct rusage rusage;
            int status;
            wait4(pid,&status,__WALL,&rusage);
            //总时间=用户态时间+内核态时间
            int tempvar_time=0;
            tempvar_time=rusage.ru_utime.tv_sec*1000+rusage.ru_utime.tv_usec/1000;
            tempvar_time+=rusage.ru_stime.tv_sec*1000+rusage.ru_stime.tv_usec/1000;
            if(result.time<tempvar_time){
                result.time=tempvar_time;
            }
            if(result.memory<rusage.ru_maxrss){
                result.memory=rusage.ru_maxrss;
            }
            char problem_out[20];
            sprintf(problem_out,"%d.out",i);
            FILE *f1=fopen((const char*)problem_out,"r");
            FILE *f2=fopen(user_out,"r");
            result.result=compare_out(f1,f2);
            fclose(f1);
            fclose(f2);
            // printf("%d\n",rusage.ru_utime.tv_sec);
            // printf("%d\n",rusage.ru_utime.tv_usec);
            // printf("%d\n",rusage.ru_stime.tv_sec);
            // printf("%d\n",rusage.ru_stime.tv_usec);
            if(result.result != __RESULT_ACCEPT__){
                break;
            }
        }
    }
    result.memory/=1024;
    if(result.time>parameter.time){
        result.result=__RESULT_TIME_LIMIT_EXCEEDED__;
    }else if(result.memory>parameter.memory){
        result.result=__RESULT_MEMORY_LIMIT_EXCEEDED__;
    }
    return result;
}
int compare_out(FILE *f1,FILE *f2){
    if(f1!=NULL && f2!=NULL){
        int c1=0,c2=0;
         do{
            c1=fgetc(f1);
            c2=fgetc(f2);
            if(c1!=c2){
                return __RESULT_WRONG_ANSWER__;
            }
        }while(c1!=EOF&&c2!=EOF);
        if(c1!=EOF||c2!=EOF){
            return __RESULT_WRONG_ANSWER__;
        }
        return __RESULT_ACCEPT__;
    }else{
        return __RESULT_SYSTEM_ERROR__;
    }
}