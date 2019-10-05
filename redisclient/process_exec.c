#include "cJSON.h"
#include "compile.h"
#include "run.h"
#include <hiredis/hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
struct compile_parameter compile_parameter;
struct run_parameter run_parameter;
struct compile_result compile_result;
struct run_result run_result;
cJSON* json;
cJSON* retjson;
cJSON* receive_src;
cJSON* receive_language;
cJSON* receive_time;
cJSON* receive_memory;
cJSON* receive_id;
cJSON* receive_problem_id;
redisContext* c;
redisReply* reply;
char err[200];
const char* WORK_DIR = "/home/tom/judge_path";
char time_str[30];
FILE *log_file;
const char *log_path;
void write_log(const char *s)
{
    time_t t;
    struct tm *lt;
    time(&t);
    lt = localtime(&t);
    sprintf(time_str,"%d-%d-%d %d:%d:%d",lt->tm_year+1900,lt->tm_mon,lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    log_file = fopen(log_path,"a");
    fprintf(log_file,"%s  %s\n",time_str,s);
    fclose(log_file);
}

/*
 * @str: source json
 * @return: Right returns 1,error returns 0, and Set the value of err.
*/
int json_decode(const char* str)
{
    json = cJSON_Parse(str);
    if (json->type != cJSON_Object) {
        // strcpy(err, "(str) JSON type error");
        write_log("(str) JSON type error");
        return 0;
    }
    receive_src = cJSON_GetObjectItem(json, "src");
    receive_language = cJSON_GetObjectItem(json, "language");
    receive_time = cJSON_GetObjectItem(json, "time");
    receive_memory = cJSON_GetObjectItem(json, "memory");
    receive_id = cJSON_GetObjectItem(json, "id");
    receive_problem_id = cJSON_GetObjectItem(json, "problem_id");
    if (!receive_src || !receive_language || !receive_memory || !receive_time || !receive_problem_id) {
        // strcpy(err, "(str) JSON key error");
        write_log("(str) JSON key error");
        return 0;
    }
    if (receive_src->type != cJSON_String || receive_language->type != cJSON_Number || receive_time->type != cJSON_Number || receive_memory->type != cJSON_Number || receive_id->type != cJSON_Number || receive_problem_id->type != cJSON_Number) {
        // strcpy(err, "(str) JSON value error");
        write_log("(str) JSON value error");
        return 0;
    }
    return 1;
}
void clear_work_dir(int run_num)
{
    char a[100];
    sprintf(a, "/bin/rm %s/run%d/*", WORK_DIR, run_num);
    system(a);
}
/*
 * @子进程的工作过程、解析json、写入文件源码、编译运行。
 * @judge_flag: 子进程的运行编号
 * @str: 源json字符串
 * @status: ==1正确、==0内部错误、==-1原json格式错误
 * @return: 结果json_str、status正确返回、错误返回NULL
*/
const char* exec_child(int judge_flag, const char* str, int* status)
{
    *status = 0;
    int run_num = judge_flag;
    if (!json_decode(str)) {
        // printf("%s\n", err);
        cJSON_Delete(json);
        *status = -1;
        return NULL;
    }
    FILE* src_file;
    char srcfile_path[100];
    char run_dir[100];
    sprintf(run_dir, "%s/run%d", WORK_DIR, run_num);
    compile_parameter.file_path = run_dir;
    if (receive_language->valueint == LANGUAGE_C) {
        sprintf(srcfile_path, "%s/run%d/main.c", WORK_DIR, run_num);
        src_file = fopen(srcfile_path, "w");
        compile_parameter.file_name = "main.c";
        compile_parameter.language = LANGUAGE_C;
    } else if (receive_language->valueint == LANGUAGE_CPP) {
        sprintf(srcfile_path, "%s/run%d/main.cpp", WORK_DIR, run_num);
        src_file = fopen(srcfile_path, "w");
        compile_parameter.file_name = "main.cpp";
        compile_parameter.language = LANGUAGE_CPP;
    } else if (receive_language->valueint == LANGUAGE_JAVA) {

    } else {
    }
    if (src_file == NULL) {
        // strcpy(err, "redis_client: Cannot open source file");
        // printf("%s\n", err);
        write_log("redis_client: Cannot open source file");
        cJSON_Delete(json);
        *status = 0;
        return NULL;
    }
    fprintf(src_file, "%s", receive_src->valuestring);
    fclose(src_file);
    compile_result = compile(compile_parameter);
    char testcase_dir[100];
    sprintf(testcase_dir, "%s/problem/%d", WORK_DIR, receive_problem_id->valueint);
    if (compile_result.right) {
        // printf("run%d compile right\n", run_num);
        write_log("编译正确");
        run_parameter.file_path = run_dir;
        run_parameter.file_name = compile_result.return_name;
        run_parameter.case_path = testcase_dir;
        run_parameter.language = receive_language->valueint;
        run_parameter.time = receive_time->valueint;
        run_parameter.memory = receive_memory->valueint;
        run_result = run(run_parameter);
        retjson = cJSON_CreateObject();
        cJSON_AddNumberToObject(retjson, "id", receive_id->valueint);
        cJSON_AddNumberToObject(retjson, "result", run_result.result);
        cJSON_AddNumberToObject(retjson, "time", run_result.time);
        cJSON_AddNumberToObject(retjson, "memory", run_result.memory);
        sprintf(err,"run%d time=%d memory=%d result=%d\n", run_num, run_result.time, run_result.memory, run_result.result);
        write_log(err);
    } else {
        // printf("run%d compile wrong\n", run_num);
        write_log("编译错误");
        retjson = cJSON_CreateObject();
        cJSON_AddNumberToObject(retjson, "id", receive_id->valueint);
        cJSON_AddNumberToObject(retjson, "result", 0);
        cJSON_AddNumberToObject(retjson, "time", 0);
        cJSON_AddNumberToObject(retjson, "memory", 0);
    }
    char compileinfo_path[100];
    sprintf(compileinfo_path, "%s/run%d/%s", WORK_DIR, run_num, compile_result.return_info_name);
    FILE* fp = fopen(compileinfo_path, "r");
    char compileinfo_str[8192];
    char line[1000];
    if (fp == NULL) {
        // strcpy(err, "open compile_info.out fail");
        // printf("%s\n", err);
        write_log("open compile_info.out fail");
        cJSON_Delete(json);
        cJSON_Delete(retjson);
        clear_work_dir(run_num);
        *status = 0;
        return NULL;
    }
    while (!feof(fp)) {
        line[0] = 0;
        fgets(line, 1000, fp);
        strcat(compileinfo_str, line);
    }
    fclose(fp);
    // printf("run%d  compile_info.out\n************%s\n**************************\n", run_num, compileinfo_str);
    write_log(compileinfo_str);
    cJSON_AddStringToObject(retjson, "compile", compileinfo_str);
    *status = 1;
    return cJSON_PrintUnformatted(retjson);
}

int main(int argc, char** argv)
{
    char log_path_arr[100];
    sprintf(log_path_arr,"%s/log/run%s.log",WORK_DIR,argv[1]);
    log_path = log_path_arr;
    write_log("运行process_exec");
    int judge_flag = atoi(argv[1]);
    const char* str = argv[2];
    WORK_DIR = argv[3];
    chdir(WORK_DIR);
    int status = 1;
    const char* resultjson_str = exec_child(judge_flag, str, &status);
    c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            // printf("run%d  Error: %s\n", judge_flag, c->errstr);
            // handle error
        } else {
            // printf("Can't allocate redis context\n");
        }
        write_log("redisConnect error");
        exit(1);
    }
    if (status != 1) {
        cJSON_Delete(retjson);
        retjson = cJSON_CreateObject();
        cJSON_AddNumberToObject(retjson, "time", 0);
        cJSON_AddNumberToObject(retjson, "memory", 0);
        cJSON_AddStringToObject(retjson, "compile", "");
        if (status == 0) {
            cJSON_AddNumberToObject(retjson, "id", receive_id->valueint);
            cJSON_AddNumberToObject(retjson, "result", -1);
        }else{
            cJSON_AddNumberToObject(retjson, "id", -1);
            cJSON_AddNumberToObject(retjson, "result", -1);
        }
        resultjson_str= cJSON_PrintUnformatted(retjson);
    }
    reply = redisCommand(c, "lpush result_json_str %s", resultjson_str);
    if (reply->type == REDIS_REPLY_ERROR) {
        // strcpy(err, reply->str);
        // printf("run%d  %s\n", judge_flag, err);
        write_log(reply->str);
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        // printf("run%d  exec success\n", judge_flag);
        write_log("exec success");
    } else {
        // strcpy(err, "reply: execute lpush decode unknown error type=");
        // printf("%s\n", err);
        // printf("run%d  type=%d\n", judge_flag, reply->type);
        write_log("type error");
    }
    cJSON_Delete(json);
    cJSON_Delete(retjson);
    clear_work_dir(judge_flag);
    freeReplyObject(reply);
    redisFree(c);
    return 0;
}