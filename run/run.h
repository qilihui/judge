#ifndef __RUN_
#define __RUN__
#define __MB__ 1048576
#define __RESULT_ACCEPT__ 1
#define __RESULT_WRONG_ANSWER__ 2
#define __RESULT_RUNNING_ERROR__ 3
#define __RESULT_TIME_LIMIT_EXCEEDED__ 4
#define __RESULT_MEMORY_LIMIT_EXCEEDED__ 5
#define __RESULT_SYSTEM_ERROR__ 6
//运行所需要的参数
struct run_parameter
{
    char *file_path;    //工作目录 应包含可执行二进制文件 测试用例的输入输出文件
    char *file_name;    //可执行文件名
    int language;   //语言类型  1c     2cpp    3java
    int case_num;   //测试用例数量
    int memory;     //内存限制 兆字节
    int time;       //时间限制 毫秒
};

//运行结果
struct run_result
{
    int time;
    int memory;
    int result;
};

//运行执行文件
struct run_result run(struct run_parameter parameter);
//用户输出与测试用例比较
int compare_out(FILE *f1,FILE *f2);

#endif