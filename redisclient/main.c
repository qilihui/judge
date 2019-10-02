#include "cJSON.h"
#include "compile.h"
#include "run.h"
#include <hiredis/hiredis.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
struct compile_parameter compile_parameter;
struct run_parameter run_parameter;
struct compile_result compile_result;
struct run_result run_result;
cJSON* json;
cJSON* retjson;
int main()
{
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
        reply = redisCommand(c, "blpop test 100");
        if (reply->type == REDIS_REPLY_ERROR) {
            printf("Error :%s\n", reply->str);
        }
        if (reply->type == REDIS_REPLY_ARRAY) {
            int num = reply->elements;
            printf("return num = %d\n", num);
            for (int i = 0; i < num; i++) {
                if (reply->element[i]->type == REDIS_REPLY_STRING) {
                    printf("%d) %s\n", i, reply->element[i]->str);
                }
            }
            json = cJSON_Parse(reply->element[1]->str);
            cJSON* receive_src = cJSON_GetObjectItem(json, "src");
            cJSON* receive_language = cJSON_GetObjectItem(json, "language");
            cJSON* receive_time = cJSON_GetObjectItem(json, "time");
            cJSON* receive_memory = cJSON_GetObjectItem(json, "memory");
            cJSON* receive_id = cJSON_GetObjectItem(json, "id");
            if(receive_src || receive_language || receive_memory || receive_time){
                printf("source json 字段错误\n");
            }
            if(receive_src->type!=cJSON_String || receive_language->type!=cJSON_Number || receive_time->type!=cJSON_Number || receive_memory->type!=cJSON_Number || receive_id->type!=cJSON_Number){
                printf("source json 格式错误\n");
            }
            FILE* src_file;
            if (receive_language->valueint == LANGUAGE_C) {
                src_file = fopen("/home/tom/work/problem/main.c", "w");
                compile_parameter.file_path = "/home/tom/work/problem";
                compile_parameter.file_name = "main.c";
                compile_parameter.language = LANGUAGE_C;
            } else if (receive_language->valueint == LANGUAGE_CPP) {

            } else if (receive_language->valueint == LANGUAGE_JAVA) {

            } else {
            }
            fprintf(src_file, "%s", receive_src->valuestring);
            fclose(src_file);
            compile_result = compile(compile_parameter);
            if (compile_result.right) {
                printf("\ncompile right\n");
                run_parameter.file_path = "/home/tom/work/problem";
                run_parameter.file_name = compile_result.return_name;
                run_parameter.case_path = "/home/tom/work/testcase";
                run_parameter.language = receive_language->valueint;
                run_parameter.time = receive_time->valueint;
                run_parameter.memory = receive_memory->valueint;
                run_result = run(run_parameter);
                retjson = cJSON_CreateObject();
                cJSON_AddNumberToObject(retjson, "id", receive_id->valueint);
                cJSON_AddNumberToObject(retjson, "result",run_result.result);
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
            FILE* fp = fopen("/home/tom/work/problem/compile_info.out", "r");
            char a[8192];
            for (int i = 0; !feof(fp); i++) {
                fscanf(fp, "%c", a + i);
            }
            fclose(fp);
            printf("\n*****compile_info.out*****\n%s\n*********\n", a);
            cJSON_Delete(json);
            cJSON_Delete(retjson);
        }
        freeReplyObject(reply);
        // sleep(2);
    }
    redisFree(c);
}