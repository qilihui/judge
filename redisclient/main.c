#include "cJSON.h"
#include "compile.h"
#include "run.h"
#include <hiredis/hiredis.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
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
char err[200];
const char *WORK_PATH = "/home/tom/judge_path";
/*
 * @str: source json
 * @return: Right returns 1,error returns 0, and Set the value of err.
*/
int json_decode(const char* str)
{
    json = cJSON_Parse(str);
    if (json->type != cJSON_Object) {
        strcpy(err,"(str) JSON type error");
        return 0;
    }
    receive_src = cJSON_GetObjectItem(json, "src");
    receive_language = cJSON_GetObjectItem(json, "language");
    receive_time = cJSON_GetObjectItem(json, "time");
    receive_memory = cJSON_GetObjectItem(json, "memory");
    receive_id = cJSON_GetObjectItem(json, "id");
    receive_problem_id = cJSON_GetObjectItem(json, "problem_id");
    if (!receive_src || !receive_language || !receive_memory || !receive_time || !receive_problem_id) {
        strcpy(err,"(str) JSON key error");
        return 0;
    }
    if (receive_src->type != cJSON_String || receive_language->type != cJSON_Number || receive_time->type != cJSON_Number || receive_memory->type != cJSON_Number || receive_id->type != cJSON_Number || receive_problem_id->type!=cJSON_Number) {
        strcpy(err,"(str) JSON value error");
        return 0;
    }
    return 1;
}
void clear_work_path(int run_num)
{
    char a[100];
    sprintf(a,"rm %s/run%d",WORK_PATH,run_num);
    system(a);
}
int main()
{
    chdir(WORK_PATH);
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Error: %s\n", c->errstr);
            // handle error
        } else {
            printf("Can't allocate redis context\n");
        }
    }
    redisReply* reply;
    while (1) {
        printf("开始执行blpop\n");
        reply = redisCommand(c, "blpop test 3");
        if (reply->type == REDIS_REPLY_NIL) {
            printf("Waiting for timeout\n");
            continue;
        }else if (reply->type == REDIS_REPLY_ERROR) {
            strcpy(err,reply->str);
            printf("%s\n", err);
        }else if (reply->type == REDIS_REPLY_ARRAY) {
            int num = reply->elements;
            printf("return num = %d\n", num);
            for (int i = 0; i < num; i++) {
                if (reply->element[i]->type == REDIS_REPLY_STRING) {
                    printf("%d) %s\n", i, reply->element[i]->str);
                }
            }

            int run_num=0;
            if(num>=2 && !json_decode(reply->element[1]->str)){
                printf("%s\n",err);
                continue;
            }
            FILE* src_file;
            char srcfile_path[100];
            char run_path[100];
            sprintf(run_path,"%s/run%d",WORK_PATH,run_num);
            compile_parameter.file_path = run_path;
            if (receive_language->valueint == LANGUAGE_C) {
                sprintf(srcfile_path,"%s/run%d/main.c",WORK_PATH,run_num);
                src_file = fopen(srcfile_path, "w");
                compile_parameter.file_name = "main.c";
                compile_parameter.language = LANGUAGE_C;
            } else if (receive_language->valueint == LANGUAGE_CPP) {

            } else if (receive_language->valueint == LANGUAGE_JAVA) {

            } else {
            }
            fprintf(src_file, "%s", receive_src->valuestring);
            fclose(src_file);
            compile_result = compile(compile_parameter);
            char testcase_path[100];
            sprintf(testcase_path,"%s/problem/%d",WORK_PATH,receive_problem_id->valueint);
            if (compile_result.right) {
                printf("\ncompile right\n");
                run_parameter.file_path = run_path;
                run_parameter.file_name = compile_result.return_name;
                run_parameter.case_path = testcase_path;
                run_parameter.language = receive_language->valueint;
                run_parameter.time = receive_time->valueint;
                run_parameter.memory = receive_memory->valueint;
                run_result = run(run_parameter);
                retjson = cJSON_CreateObject();
                cJSON_AddNumberToObject(retjson, "id", receive_id->valueint);
                cJSON_AddNumberToObject(retjson, "result", run_result.result);
                cJSON_AddNumberToObject(retjson, "time", run_result.time);
                cJSON_AddNumberToObject(retjson, "memory", run_result.memory);
                printf("\n******run end\ntime=%d\nmemory=%d\nresult=%d\n********\n", run_result.time, run_result.memory, run_result.result);
            } else {
                printf("\ncompile wrong\n");
                retjson = cJSON_CreateObject();
                cJSON_AddNumberToObject(retjson, "id", receive_id->valueint);
                cJSON_AddNumberToObject(retjson, "result", 0);
                cJSON_AddNumberToObject(retjson, "time", 0);
                cJSON_AddNumberToObject(retjson, "memory", 0);
            }
            char compileinfo_path[100];
            sprintf(compileinfo_path,"%s/run%d/%s",WORK_PATH,run_num,compile_result.return_name);
            FILE* fp = fopen(compileinfo_path, "r");
            char a[8192];
            for (int i = 0; !feof(fp); i++) {
                fscanf(fp, "%c", a + i);
            }
            fclose(fp);
            printf("\n*****compile_info.out*****\n%s\n*********\n", a);
            cJSON_Delete(json);
            cJSON_Delete(retjson);
            clear_work_path(run_num);

        }else{
            strcpy(err,"return reply: unknown error");
            printf("%s\n",err);
        }
        freeReplyObject(reply);
        // sleep(2);
    }
    redisFree(c);
}