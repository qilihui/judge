#ifndef __COMPILE__
#define __COMPILE__
#define LANGUAGE_C 1
#define LANGUAGE_CPP 2
#define LANGUAGE_JAVA 3
//传入参数
struct compile_parameter
{
    const char *file_path;    //工作目录 所要编译文件位置
    const char *file_name;    //文件名
    int language;   //语言类型  1c     2cpp    3java
};

//返回结果
struct compile_result
{
    int right;  //正确？    1正确   0错误
    // char *return_path;  //编译器返回信息路径
    const char *return_name;  //返回信息文件名
};

struct compile_result compile(struct compile_parameter parameter);

#endif