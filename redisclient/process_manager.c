#include "cJSON.h"
#include "compile.h"
#include "run.h"
#include <hiredis/hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

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
char time_str[30];
FILE* log_file;
const char* log_path;
void write_log(const char* s)
{
    time_t t;
    struct tm* lt;
    time(&t);
    lt = localtime(&t);
    sprintf(time_str, "%d-%d-%d %d:%d:%d", lt->tm_year + 1990, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    log_file = fopen(log_path, "a");
    fprintf(log_file, "%s  %s\n", time_str, s);
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

int main()
{
    char log_path_arr[100];
    sprintf(log_path_arr, "%s/log/manager.log", WORK_DIR);
    log_path = log_path_arr;
    write_log("运行process_manager");
    c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            // printf("Error: %s\n", c->errstr);
            write_log(c->errstr);
            // handle error
        } else {
            // printf("Can't allocate redis context\n");
            write_log("Can't allocate redis context");
        }
    }
    while (1) {
        // printf("开始执行\n");
        write_log("开始执行while循环");
        reply = redisCommand(c, "brpop source_json_str 100");
        if (reply->type == REDIS_REPLY_NIL) {
            // printf("Waiting for timeout\n");
            write_log("Waiting for timeout");
            freeReplyObject(reply);
            continue;
        } else if (reply->type == REDIS_REPLY_ERROR) {
            // strcpy(err, reply->str);
            // printf("%s\n", err);
            write_log(reply->str);
            freeReplyObject(reply);
            continue;
        } else if (reply->type == REDIS_REPLY_ARRAY) {
            int num = reply->elements;
            int i = 0;
            for (i = 0; i < num; i++) {
                if (reply->element[i]->type == REDIS_REPLY_STRING) {
                    // printf("%d) %s\n", i, reply->element[i]->str);
                    write_log(reply->element[1]->str);
                } else {
                    break;
                }
            }
            if (num < 2 || i != num) {
                // strcpy(err, "redisclient: brpop value < 2");
                // printf("%s", err);
                write_log("redisclient: brpop value < 2 || num!=STRING");
                freeReplyObject(reply);
                continue;
            }
            pid_t end_process = 0;
            while ((end_process = waitpid(-1, NULL, WNOHANG)) > 0) {
                for (int i = 0; i < judge_num; i++) {
                    if (end_process == judge[i]) {
                        judge[i] = 0;
                        break;
                    }
                }
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
                char judge_flag_str[3];
                sprintf(judge_flag_str, "%d", judge_flag);
                sprintf(err, "./process_exec %s %s %s", judge_flag_str, reply->element[1]->str, WORK_DIR);
                write_log(err);
                execlp("/home/tom/work/redisclient/process_exec", "process_exec", judge_flag_str, reply->element[1]->str, WORK_DIR, NULL);
            } else if (pid > 0) {
                judge[judge_flag] = pid;
            }
            freeReplyObject(reply);
            continue;
        } else {
            strcpy(err, "reply: execute brpop decode json unknown error");
            // printf("%s\n", err);
            // printf("type = %d\n", reply->type);
            write_log("error type");
            freeReplyObject(reply);
            continue;
        }
    }
    redisFree(c);
}