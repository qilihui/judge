#include "run.h"
#include "write_log.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
const char* user_ip = "";
const char* user_port = "";
const char* user_user = "";
const char* user_passwd = "";
const char* user_dbname = "";
char mysql_ip_arr[20];
char mysql_port_arr[10];
char mysql_user_arr[20];
char mysql_passwd_arr[20];
char mysql_dbname_arr[20];

int compare_out(FILE* f1, FILE* f2)
{
    if (f1 != NULL && f2 != NULL) {
        int c1 = 0, c2 = 0;
        do {
            if (c1 != c2) {
                return __RESULT_WRONG_ANSWER__;
            }
            c1 = fgetc(f1);
            c2 = fgetc(f2);
        } while (c1 != EOF && c2 != EOF);

        if (c1 != EOF && c2 == EOF) {
            return __RESULT_WRONG_ANSWER__;
        } else if (c1 != EOF && c2 != EOF) {
            return __RESULT_WRONG_ANSWER__;
        } else if (c1 == EOF && c2 != EOF) {
            do {
                if (c2 != ' ' && c2 != '\n') {
                    return __RESULT_WRONG_ANSWER__;
                }
                c2 = fgetc(f2);
            } while (c2 != EOF);
            return __RESULT_ACCEPT__;
        } else {
            return __RESULT_ACCEPT__;
        }

    } else {
        return __RESULT_SYSTEM_ERROR__;
    }
}

/*
 *复制动态库文件到当前目录
 */
void cp_lib()
{
    system("mkdir -p ./usr/lib/x86_64-linux-gnu");
    system("mkdir -p ./lib/x86_64-linux-gnu");
    system("mkdir -p ./lib64");
    system("cp /usr/lib/x86_64-linux-gnu/libstdc++.so.6 ./usr/lib/x86_64-linux-gnu/");
    system("cp /lib/x86_64-linux-gnu/libm.so.6 ./lib/x86_64-linux-gnu/");
    system("cp /lib/x86_64-linux-gnu/libgcc_s.so.1 ./lib/x86_64-linux-gnu/");
    system("cp /lib/x86_64-linux-gnu/libc.so.6 ./lib/x86_64-linux-gnu/");
    system("cp /lib64/ld-linux-x86-64.so.2 ./lib64/");
    system("chmod 777 -R ./*");
}

void init_result(struct run_result* result)
{
    result->result = __RESULT_ACCEPT__;
    result->time = 0;
    result->memory = 0;
    result->exit_sig = 0;
    result->exit_code = 0;
}

/*
 *加载配置文件 位置是 /judge.conf
 */
void load_run_conf()
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
        case 5:
            strcpy(mysql_ip_arr, token);
            break;
        case 6:
            strcpy(mysql_port_arr, token);
            break;
        case 7:
            strcpy(mysql_user_arr, token);
            break;
        case 8:
            strcpy(mysql_passwd_arr, token);
            break;
        case 9:
            strcpy(mysql_dbname_arr, token);
            break;
        default:
            break;
        }
    }
    fclose(conf_fp);
    user_ip = mysql_ip_arr;
    user_port = mysql_port_arr;
    user_user = mysql_user_arr;
    user_passwd = mysql_passwd_arr;
    user_dbname = mysql_dbname_arr;
}

/*
 * 字符串替换
 */
char* strrpc(char* str, char* oldstr, char* newstr)
{
    char bstr[strlen(str)]; //转换缓冲区
    memset(bstr, 0, sizeof(bstr));

    for (int i = 0; i < strlen(str); i++) {
        if (!strncmp(str + i, oldstr, strlen(oldstr))) { //查找目标字符串
            strcat(bstr, newstr);
            i += strlen(oldstr) - 1;
        } else {
            strncat(bstr, str + i, 1); //保存一字节进缓冲区
        }
    }

    strcpy(str, bstr);
    return str;
}

/*
 * 去除字符串结尾空格 换行
 */
char* strrtrim(char* str)
{
    if (str == NULL || *str == '\0') {
        return str;
    }
    int len = strlen(str);
    char* p = str + len - 1;
    while (p >= str && (*p == ' ' || *p == '\n')) {
        *p = '\0';
        --p;
    }

    return str;
}

int load_case(struct run_parameter parameter)
{
    if (parameter.debug_mode)
        write_log(parameter.log_path, "从mysql获取测试用例");
    int n = mkdir(parameter.case_path, 00775);
    if (n) {
        // printf("%s\n", strerror(errno));
        write_log(parameter.log_path, strerror(errno));
    }
    load_run_conf();
    MYSQL* conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, user_ip, user_user, user_passwd, user_dbname, atoi(user_port), 0, 0)) {
        write_log(parameter.log_path, "mysql连接失败");
        conn = NULL;
        return -1;
    }
    char sql[100];
    sprintf(sql, "select in_case,out_case from test_case where test_case_id=%d", parameter.case_id);
    if (mysql_real_query(conn, sql, strlen(sql))) {
        write_log(parameter.log_path, "执行sql语句失败");
        mysql_close(conn);
        return -1;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;
    int i = 0;
    char file_path_arr[100];
    FILE* fp;
    while (res != NULL && (row = mysql_fetch_row(res)) != NULL) {
        ++i;
        // printf("*****in*****\n%s\n*****out*****\n%s\n\n", row[0], row[1]);
        sprintf(file_path_arr, "%s/%d.in", parameter.case_path, i);
        fp = fopen(file_path_arr, "w");
        if (fp == NULL) {
            write_log(parameter.log_path, "打开文件写入测试用例 打开文件失败");
            return -1;
        }
        char* hin = (char*)malloc(sizeof(char) * (strlen(row[0]) + 1));
        memset(hin, 0, sizeof(hin));
        strcpy(hin, row[0]);
        strrpc(hin, "\r\n", "\n");
        strrtrim(hin);
        fprintf(fp, "%s", hin);
        fclose(fp);
        free(hin);

        sprintf(file_path_arr, "%s/%d.out", parameter.case_path, i);
        fp = fopen(file_path_arr, "w");
        if (fp == NULL) {
            write_log(parameter.log_path, "打开文件写入测试用例 打开文件失败");
            return -1;
        }
        char* hout = (char*)malloc(sizeof(char) * (strlen(row[1]) + 1));
        memset(hout, 0, sizeof(hout));
        strcpy(hout, row[1]);
        strrpc(hout, "\r\n", "\n");
        strrtrim(hout);
        fprintf(fp, "%s", hout);
        fclose(fp);
        free(hout);
    }
    mysql_close(conn);
    mysql_free_result(res);
    conn = NULL;
    if (i == 0) {
        write_log(parameter.log_path, "没有在mysql中查找到测试用例");
        return -1;
    }
    return 0;
}

struct run_result run(struct run_parameter parameter)
{
    struct run_result result;
    init_result(&result);
    //跳转到工作目录
    chdir((const char*)parameter.file_path);
    cp_lib();
    if (parameter.debug_mode)
        write_log(parameter.log_path, "复制动态库完成 进入run函数");
    const char* user_out = "user.out";
    const char* case_path = parameter.case_path;

    for (int i = 1; 1; i++) {
        char input_name[50];
        sprintf(input_name, "%s/%d.in", case_path, i);
        if (access((const char*)input_name, F_OK) == -1) {
            if (i == 1) {
                if (load_case(parameter)) {
                    result.result = __RESULT_SYSTEM_ERROR__;
                    return result;
                }
                if (parameter.debug_mode)
                    write_log(parameter.log_path, "mysql 执行完成");
                // exit(0);
            } else {
                break;
            }
        }

        pid_t pid = fork();
        if (pid == 0) {
            if (parameter.debug_mode)
                write_log(parameter.log_path, "进入run函数 子进程");
            struct rlimit lim;
            lim.rlim_cur = lim.rlim_max = parameter.time / 1000 + 1;
            setrlimit(RLIMIT_CPU, &lim);
            lim.rlim_cur = lim.rlim_max = parameter.memory * __MB__ * 2;
            setrlimit(RLIMIT_AS, &lim);
            lim.rlim_cur = lim.rlim_max = __MB__;
            setrlimit(RLIMIT_FSIZE, &lim);
            lim.rlim_cur = lim.rlim_max = 1;
            setrlimit(RLIMIT_NPROC, &lim);

            if (freopen((const char*)input_name, "r", stdin) == NULL) {
                write_log(parameter.log_path, "进入run函数 子进程 打开stdin出错");
                exit(3);
            }

            if (freopen(user_out, "w", stdout) == NULL) {
                write_log(parameter.log_path, "进入run函数 子进程 打开stdout出错");
                exit(3);
            }

            if (freopen("err.out", "w", stderr) == NULL) {
                write_log(parameter.log_path, "进入run函数 子进程 打开stderr出错");
                exit(3);
            }

            char x[100];
            getcwd(x, sizeof(x));
            if (parameter.debug_mode)
                write_log(parameter.log_path, x);
            if (parameter.debug_mode)
                write_log(parameter.log_path, "执行chroot");
            if (chroot(parameter.file_path)) {
                printf("%s\n", strerror(errno));
                write_log(parameter.log_path, strerror(errno));
                exit(3);
            }
            if (execl(parameter.file_name, parameter.file_name, NULL) == -1) {
                exit(3);
            }
            exit(0);

        } else {
            struct rusage rusage;
            int status;
            wait4(pid, &status, __WALL, &rusage);

            //总时间=用户态时间+内核态时间
            int tempvar_time = 0;
            tempvar_time = rusage.ru_utime.tv_sec * 1000 + rusage.ru_utime.tv_usec / 1000;
            tempvar_time += rusage.ru_stime.tv_sec * 1000 + rusage.ru_stime.tv_usec / 1000;
            if (result.time < tempvar_time) {
                result.time = tempvar_time;
            }

            if (result.memory < rusage.ru_maxrss) {
                result.memory = rusage.ru_maxrss;
            }

            //收到信号退出 代表运行错误
            if (WIFSIGNALED(status)) {
                result.exit_sig = WTERMSIG(status);
                result.result = __RESULT_RUNNING_ERROR__;
                break;
            }

            //子进程执行exit
            if (WIFEXITED(status)) {
                //异常退出  exit(!0)
                if (WEXITSTATUS(status)) {
                    result.exit_code = WEXITSTATUS(status);
                    result.result = __RESULT_SYSTEM_ERROR__;
                    break;
                }
            }

            //其他情况为正常return退出 和 exit(0)退出

            char log_arr[20];
            sprintf(log_arr, "执行status=%d", status >> 8);
            if (parameter.debug_mode)
                write_log(parameter.log_path, log_arr);
            char problem_out[50];
            sprintf(problem_out, "%s/%d.out", case_path, i);

            //打开测试用例输出和用户输出比较
            FILE* f1 = fopen((const char*)problem_out, "r");
            FILE* f2 = fopen(user_out, "r");
            result.result = compare_out(f1, f2);
            if (parameter.debug_mode)
                write_log(parameter.log_path, "进入run函数 文件比较结束");
            fclose(f1);
            fclose(f2);

            if (result.result != __RESULT_ACCEPT__) {
                break;
            }
        }
    }

    result.memory /= 1024;
    if (result.time > parameter.time) {
        result.result = __RESULT_TIME_LIMIT_EXCEEDED__;
    } else if (result.memory > parameter.memory) {
        result.result = __RESULT_MEMORY_LIMIT_EXCEEDED__;
    }
    return result;
}