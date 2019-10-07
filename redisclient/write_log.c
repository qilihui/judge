#include <time.h>
#include <stdio.h>
#include "write_log.h"
void write_log(const char* log_path,const char* s)
{
    FILE *log_file;
    char time_str[30];
    time_t t;
    struct tm* lt;
    time(&t);
    lt = localtime(&t);
    sprintf(time_str, "%d-%d-%d %d:%d:%d", lt->tm_year + 1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    log_file = fopen(log_path, "a");
    fprintf(log_file, "%s  %s\n", time_str, s);
    if(log_file!=NULL)
        fclose(log_file);
}