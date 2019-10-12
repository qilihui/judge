#include "cJSON.h"
#include "compile.h"
#include "run.h"
#include "write_log.h"
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
const char* WORK_DIR = "/judge_path";
int judge_num = 3;
int judge[10] = { 0 };
const char* log_path;
const char* redis_ip = "127.0.0.1";
const char* redis_port = "6379";

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

void make_dir()
{
    char a[100] = { 0 };
    for (int i = 0; i < judge_num; i++) {
        sprintf(a, "mkdir %s/run%d 2>/dev/null", WORK_DIR, i);
        system(a);
    }
}

int main()
{
    FILE* conf_fp = fopen("./judge.conf", "r");
    char conf_arr[100];
    char work_dir_arr[100];
    char redis_ip_arr[20];
    char redis_port_arr[10];

    if (conf_fp == NULL) {
        exit(1);
    }

    char* token;
    for (int i = 0; !feof(conf_fp); i++) {
        conf_arr[0] = 0;
        fgets(conf_arr, 99, conf_fp);
        strtok(conf_arr, "\"");
        token = strtok(NULL, "\"");
        switch (i) {
        case 0:
            strcpy(redis_ip_arr, token);
            break;
        case 1:
            strcpy(redis_port_arr, token);
            break;
        case 2:
            strcpy(work_dir_arr, token);
            break;
        case 3:
            judge_num = atoi(token);
            break;
        default:
            break;
        }
    }
    fclose(conf_fp);

    WORK_DIR = work_dir_arr;
    redis_ip = redis_ip_arr;
    redis_port = redis_port_arr;
    make_dir();
    char log_path_arr[100];
    sprintf(log_path_arr, "%s/log/manager.log", WORK_DIR);
    log_path = log_path_arr;
    write_log(log_path, "运行process_manager");
    write_log(log_path, redis_ip);
    write_log(log_path, redis_port);
    write_log(log_path, WORK_DIR);

    while (1) {
        if (c == NULL) {
            c = redisConnect(redis_ip, atoi(redis_port));
            if (c == NULL || c->err) {
                if (c) {
                    write_log(log_path, c->errstr);
                } else {
                    write_log(log_path, "Can't allocate redis context");
                }
                continue;
            }
        }

        write_log(log_path, "开始执行while循环");
        pid_t end_process = 0;

        while ((end_process = waitpid(-1, NULL, WNOHANG)) > 0) {
            for (int i = 0; i < judge_num; i++) {
                if (end_process == judge[i]) {
                    judge[i] = 0;
                    break;
                }
            }
        }

        reply = redisCommand(c, "brpop source_json_str 100");
        if (reply->type == REDIS_REPLY_NIL) {
            write_log(log_path, "Waiting for timeout");
        } else if (reply->type == REDIS_REPLY_ERROR) {
            write_log(log_path, reply->str);
        } else if (reply->type == REDIS_REPLY_ARRAY) {
            int num = reply->elements;
            int i = 0;
            for (i = 0; i < num; i++) {
                if (reply->element[i]->type != REDIS_REPLY_STRING) {
                    break;
                }
            }

            if (num < 2 || i != num) {
                write_log(log_path, "redisclient: brpop value < 2 || num!=STRING");
                freeReplyObject(reply);
                continue;
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
                sprintf(err, "子进程执行   ./process_exec %s %s %s %s %s", judge_flag_str, reply->element[1]->str, WORK_DIR, redis_ip, redis_port);
                write_log(log_path, err);
                int res = execlp("./process_exec", "process_exec", judge_flag_str, reply->element[1]->str, WORK_DIR, redis_ip, redis_port, NULL);
                if (res == -1)
                    write_log(log_path, "执行execlp错误");
            } else if (pid > 0) {
                judge[judge_flag] = pid;
            }

        } else {
            strcpy(err, "reply: execute brpop decode json unknown error");
            write_log(log_path, "error type");
        }

        if (reply != NULL) {
            freeReplyObject(reply);
        }
    }
}