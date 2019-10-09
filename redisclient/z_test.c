#include<stdio.h>
#include"run.h"
int main()
{
    struct run_parameter run_parameter;
    struct run_result run_result;
    run_parameter.case_path="/home/tom/judge_path/problem/2";
    run_parameter.file_path="/home/tom/judge_path/run0";
    run_parameter.file_name="main";
    run_parameter.language=1;
    run_parameter.log_path="/home/tom/judge_path/log/test.log";
    run_parameter.memory=10;
    run_parameter.time=10000;
    run_result=run(run_parameter);
    printf("result=%d\nmemory=%d\ntime=%d\n",run_result.result,run_result.memory,run_result.time);
}