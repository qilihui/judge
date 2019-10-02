#include<string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
int main()
{
    FILE* fp = fopen("/home/tom/judge_path/run0/compile_info.out", "r");
    int a[10000];
    char line[1000];
    while (!feof(fp)) {
        // fgets(line, 1000, fp);
        fscanf(fp, "%s", line);
        // printf("%s", line);
        strcat(a,line);
    }
    printf("%s",a);
}