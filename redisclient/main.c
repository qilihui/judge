#include "cJSON.h"
#include "compile.h"
#include "run.h"
#include <hiredis/hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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
int judge_num = 3;
int judge[10] = { 0 };
/*
 * @str: source json
 * @return: Right returns 1,error returns 0, and Set the value of err.
*/
int json_decode(const char* str)
{
    json = cJSON_Parse(str);
    if (json->type != cJSON_Object) {
        strcpy(err, "(str) JSON type error");
        return 0;
    }
    receive_src = cJSON_GetObjectItem(json, "src");
    receive_language = cJSON_GetObjectItem(json, "language");
    receive_time = cJSON_GetObjectItem(json, "time");
    receive_memory = cJSON_GetObjectItem(json, "memory");
    receive_id = cJSON_GetObjectItem(json, "id");
    receive_problem_id = cJSON_GetObjectItem(json, "problem_id");
    if (!receive_src || !receive_language || !receive_memory || !receive_time || !receive_problem_id) {
        strcpy(err, "(str) JSON key error");
        return 0;
    }
    if (receive_src->type != cJSON_String || receive_language->type != cJSON_Number || receive_time->type != cJSON_Number || receive_memory->type != cJSON_Number || receive_id->type != cJSON_Number || receive_problem_id->type != cJSON_Number) {
        strcpy(err, "(str) JSON value error");
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
void exec_child(int judge_flag, char* str)
{
    int run_num = judge_flag;
    if (!json_decode(str)) {
        printf("%s\n", err);
        cJSON_Delete(json);
        exit(1);
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

    } else if (receive_language->valueint == LANGUAGE_JAVA) {

    } else {
    }
    if (src_file == NULL) {
        strcpy(err, "redis_client: Cannot open source file");
        printf("%s\n", err);
        cJSON_Delete(json);
        exit(2);
    }
    fprintf(src_file, "%s", receive_src->valuestring);
    fclose(src_file);
    compile_result = compile(compile_parameter);
    char testcase_dir[100];
    sprintf(testcase_dir, "%s/problem/%d", WORK_DIR, receive_problem_id->valueint);
    if (compile_result.right) {
        printf("compile right\n");
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
        printf("******run end\ntime=%d\nmemory=%d\nresult=%d\n********\n", run_result.time, run_result.memory, run_result.result);
    } else {
        printf("compile wrong\n");
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
        strcpy(err, "open compile_info.out fail");
        printf("%s\n", err);
        cJSON_Delete(json);
        cJSON_Delete(retjson);
        clear_work_dir(run_num);
        exit(3);
    }
    while (!feof(fp)) {
        line[0] = 0;
        fgets(line, 1000, fp);
        strcat(compileinfo_str, line);
    }
    fclose(fp);
    printf("*****compile_info.out*****\n%s\n*********\n", compileinfo_str);
    cJSON_AddStringToObject(retjson, "compile", compileinfo_str);
    char* resultjson_str = cJSON_PrintUnformatted(retjson);
    freeReplyObject(reply);
    reply = redisCommand(c, "lpush result_json_str %s", resultjson_str);
    if (reply->type == REDIS_REPLY_ERROR) {
        strcpy(err, reply->str);
        printf("%s\n", err);
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        printf("exec success\n");
    } else {
        strcpy(err, "reply: execute lpush decode unknown error");
        printf("%s\n", err);
    }
    compileinfo_str[0] = 0;
    cJSON_Delete(json);
    cJSON_Delete(retjson);
    clear_work_dir(run_num);
    exit(0);
}
int main()
{
    chdir(WORK_DIR);
    c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Error: %s\n", c->errstr);
            // handle error
        } else {
            printf("Can't allocate redis context\n");
        }
    }
    while (1) {
        printf("\n\n开始执行brpop\n");
        reply = redisCommand(c, "brpop source_json_str 100");
        if (reply->type == REDIS_REPLY_NIL) {
            printf("Waiting for timeout\n");
            freeReplyObject(reply);
            continue;
        } else if (reply->type == REDIS_REPLY_ERROR) {
            strcpy(err, reply->str);
            printf("%s\n", err);
        } else if (reply->type == REDIS_REPLY_ARRAY) {
            int num = reply->elements;
            printf("return num = %d\n", num);
            for (int i = 0; i < num; i++) {
                if (reply->element[i]->type == REDIS_REPLY_STRING) {
                    printf("%d) %s\n", i, reply->element[i]->str);
                }
            }
            if (num < 2) {
                strcpy(err, "redisclient: brpop value < 2");
                printf("%s", err);
            }
            pid_t pid = 0;
            int judge_flag = 0;
            for (; judge_flag < judge_num; judge_flag++) {
                if (judge[judge_flag] == 0)
                    break;
            }
            if (judge_flag == judge_num) {
                int status = 0;
                pid_t end_process = waitpid(-1, &status, 0);
                for (int i = 0; i < judge_num; i++) {
                    if (judge[i] == end_process) {
                        judge[i] = 0;
                        judge_flag = i;
                    }
                }
            }
            pid = fork();
            if (pid == 0) {
                exec_child(judge_flag, reply->element[1]->str);
            } else if (pid > 0) {
                judge[judge_flag] = pid;
            } else {
            }
            pid_t end_process = 0;
            while (end_process = waitpid(-1, NULL, WNOHANG) > 0) {
                for (int i = 0; i < judge_num; i++) {
                    if (end_process == judge[i]) {
                        judge[i] = 0;
                        break;
                    }
                }
            }

        } else {
            strcpy(err, "reply: execute blpop decode json unknown error");
            printf("%s\n", err);
        }
        freeReplyObject(reply);
        // sleep(2);
    }
    redisFree(c);
}