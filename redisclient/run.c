#include "run.h"
#include "write_log.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int compare_out(FILE* f1, FILE* f2)
{
    if (f1 != NULL && f2 != NULL) {
        int c1 = 0, c2 = 0;
        do {
            c1 = fgetc(f1);
            c2 = fgetc(f2);
            if (c1 != c2) {
                return __RESULT_WRONG_ANSWER__;
            }
        } while (c1 != EOF && c2 != EOF);
        if (c1 != EOF || c2 != EOF) {
            return __RESULT_WRONG_ANSWER__;
        }
        return __RESULT_ACCEPT__;
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
struct run_result run(struct run_parameter parameter)
{
    struct run_result result;
    //跳转到工作目录
    chdir((const char*)parameter.file_path);
    cp_lib();
    write_log(parameter.log_path, "复制动态库完成 进入run函数");
    result.memory = 0;
    result.result = 0;
    result.time = 0;
    const char* user_out = "user.out";
    const char* case_path = parameter.case_path;
    for (int i = 1; 1; i++) {
        char input_name[50];
        sprintf(input_name, "%s/%d.in", case_path, i);
        if (access((const char*)input_name, F_OK) == -1)
            break;
        pid_t pid = fork();
        if (pid == 0) {
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
            write_log(parameter.log_path, x);
            write_log(parameter.log_path, "执行chroot");
            if (chroot(parameter.file_path)) {
                printf("%s\n", strerror(errno));
                write_log(parameter.log_path, strerror(errno));
                exit(3);
            }
            // printf("chroot 之后\n");
            // getcwd(x, sizeof(x));
            // write_log(parameter.log_path,x);
            // write_log(parameter.log_path,"写入文件目录");
            // write_log(parameter.log_path,"chroot完成");
            // printf("%s\n",x);
            if (execl(parameter.file_name, parameter.file_name, NULL) == -1) {
                printf("%s\n", strerror(errno));
                // char a[100];
                // sprintf(a, "%s", strerror(errno));
                // write_log(parameter.log_path, a);
                exit(3);
            }
            exit(0);
            // system("pwd");
            // system("./main");
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
            if (status >> 8 == 3) {
                result.result = __RESULT_SYSTEM_ERROR__;
                result.memory = 0;
                result.time = 0;
                return result;
            }
            char log_arr[20];
            sprintf(log_arr, "执行status=%d", status >> 8);
            write_log(parameter.log_path, log_arr);
            char problem_out[50];
            sprintf(problem_out, "%s/%d.out", case_path, i);
            FILE* f1 = fopen((const char*)problem_out, "r");
            FILE* f2 = fopen(user_out, "r");
            result.result = compare_out(f1, f2);
            write_log(parameter.log_path, "进入run函数 文件比较结束");
            fclose(f1);
            fclose(f2);
            // printf("%d\n",rusage.ru_utime.tv_sec);
            // printf("%d\n",rusage.ru_utime.tv_usec);
            // printf("%d\n",rusage.ru_stime.tv_sec);
            // printf("%d\n",rusage.ru_stime.tv_usec);
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