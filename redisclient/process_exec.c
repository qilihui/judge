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
char work_dir_arr[100];
char redis_ip_arr[20];
char redis_port_arr[10];
char redis_passwd_arr[30];
const char* WORK_DIR = "/judge_path";
const char* log_path;
const char* redis_ip;
const char* redis_port;
const char* redis_passwd;
/*
 * @str: source json
 * @return: Right returns 1,error returns 0, and Set the value of err.
*/
int json_decode(const char* str)
{
    json = cJSON_Parse(str);
    if (json->type != cJSON_Object) {
        write_log(log_path, "(str) JSON type error");
        return 0;
    }
    receive_src = cJSON_GetObjectItem(json, "src");
    receive_language = cJSON_GetObjectItem(json, "language");
    receive_time = cJSON_GetObjectItem(json, "time");
    receive_memory = cJSON_GetObjectItem(json, "memory");
    receive_id = cJSON_GetObjectItem(json, "id");
    receive_problem_id = cJSON_GetObjectItem(json, "problem_id");
    if (!receive_src || !receive_language || !receive_memory || !receive_time || !receive_problem_id) {
        write_log(log_path, "(str) JSON key error");
        return 0;
    }
    if (receive_src->type != cJSON_String || receive_language->type != cJSON_Number || receive_time->type != cJSON_Number || receive_memory->type != cJSON_Number || receive_id->type != cJSON_Number || receive_problem_id->type != cJSON_Number) {
        write_log(log_path, "(str) JSON value error");
        return 0;
    }
    return 1;
}
void clear_work_dir(int run_num)
{
    char a[100];
    sprintf(a, "/bin/rm -rf %s/run%d/*", WORK_DIR, run_num);
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
        cJSON_Delete(json);
        *status = -1;
        return NULL;
    }

    write_log(log_path, "json解析成功");
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
        write_log(log_path, "redis_client: Cannot open source file");
        cJSON_Delete(json);
        *status = 0;
        return NULL;
    }

    fprintf(src_file, "%s", receive_src->valuestring);
    fclose(src_file);
    write_log(log_path, "源码写入文件成功");
    compile_result = compile(compile_parameter);
    char testcase_dir[100];
    sprintf(testcase_dir, "%s/problem/%d", WORK_DIR, receive_problem_id->valueint);

    if (compile_result.right) {
        write_log(log_path, "编译正确");
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
        cJSON_AddNumberToObject(retjson, "exit_sig", run_result.exit_sig);
        cJSON_AddNumberToObject(retjson, "exit_code", run_result.exit_code);
        sprintf(err, "run%d time=%d memory=%d result=%d exit_sig=%d exit_code=%d", run_num, run_result.time, run_result.memory, run_result.result, run_result.exit_sig, run_result.exit_code);
        write_log(log_path, err);
    } else {
        write_log(log_path, "编译错误");
        retjson = cJSON_CreateObject();
        cJSON_AddNumberToObject(retjson, "id", receive_id->valueint);
        cJSON_AddNumberToObject(retjson, "result", 0);
        cJSON_AddNumberToObject(retjson, "time", 0);
        cJSON_AddNumberToObject(retjson, "memory", 0);
        cJSON_AddNumberToObject(retjson, "exit_sig", 0);
        cJSON_AddNumberToObject(retjson, "exit_code", 0);
    }

    char compileinfo_path[100];
    sprintf(compileinfo_path, "%s/run%d/%s", WORK_DIR, run_num, compile_result.return_info_name);
    FILE* fp = fopen(compileinfo_path, "r");
    char compileinfo_str[8192];
    char line[1000];

    if (fp == NULL) {
        write_log(log_path, "open compile_info.out fail");
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

    write_log(log_path, compileinfo_str);
    cJSON_AddStringToObject(retjson, "compile", compileinfo_str);
    *status = 1;
    return cJSON_PrintUnformatted(retjson);
}

/*
 *加载配置文件 位置是 /judge.conf
 */
void load_conf()
{
    FILE* conf_fp = fopen("/judge.conf", "r");
    char conf_arr[100];

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
            strcpy(redis_passwd_arr, token);
            break;
        case 3:
            strcpy(work_dir_arr, token);
            break;
        default:
            break;
        }
    }
    fclose(conf_fp);
    WORK_DIR = work_dir_arr;
    redis_ip = redis_ip_arr;
    redis_port = redis_port_arr;
    redis_passwd = redis_passwd_arr;
}
int main(int argc, char** argv)
{
    int judge_flag = atoi(argv[1]);
    const char* str = argv[2];
    load_conf();
    chdir(WORK_DIR);
    char log_path_arr[100];
    sprintf(log_path_arr, "%s/log/run%s.log", WORK_DIR, argv[1]);
    log_path = log_path_arr;
    compile_parameter.log_path = log_path;
    run_parameter.log_path = log_path;
    write_log(log_path, "运行process_exec");
    int status = 1;
    const char* resultjson_str = exec_child(judge_flag, str, &status);
    write_log(log_path, "执行完成 开始连接redis");

    for (int i = 0; i < 3; i++) {
        c = redisConnect(redis_ip, atoi(redis_port));
        if (c == NULL) {
            write_log(log_path, "redisConnect == NULL");
            continue;
        }
        if (c->err) {
            write_log(log_path, c->errstr);
            redisFree(c);
            c = NULL;
            continue;
        }
        break;
    }

    //认证
    reply = redisCommand(c, "AUTH %s", redis_passwd);
    if (reply == NULL) {
        write_log(log_path, "redis 认证失败-reply为空");
        redisFree(c);
        c = NULL;
        exit(0);
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        write_log(log_path, reply->str);
        exit(0);
    } else {
        write_log(log_path, "redis 认证成功");
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
        } else {
            cJSON_AddNumberToObject(retjson, "id", -1);
            cJSON_AddNumberToObject(retjson, "result", -1);
        }
        resultjson_str = cJSON_PrintUnformatted(retjson);
    }

    reply = redisCommand(c, "lpush result_json_str %s", resultjson_str);

    if (reply == NULL) {
        exit(0);
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        write_log(log_path, reply->str);
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        write_log(log_path, "exec success");
    } else {
        write_log(log_path, "type error");
    }

    cJSON_Delete(json);
    cJSON_Delete(retjson);
    freeReplyObject(reply);
    clear_work_dir(judge_flag);
    redisFree(c);
    return 0;
}