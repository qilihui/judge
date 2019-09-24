#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "compile.h"

#define work_path "/home/tom/work/run"

int main(int argc,char **argv)
{
    struct compile_parameter parameter;
    struct compile_result result;
    parameter.file_path = argv[1];
    parameter.file_name = argv[2];
    parameter.language = atoi(argv[3]);
    result = compile(parameter);
}