#include "cJSON.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

int sockfd;
struct sockaddr_in ser, cli;

void print_log(const char* s)
{
    printf("%s: %s\n", s,strerror(errno));
}
void init_listen()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        print_log("socket error");
        exit(1);
    }
    ser.sin_family = AF_INET;
    ser.sin_port = htons(80);
    ser.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(sockfd, (struct sockaddr*)&ser, sizeof(ser)) == -1) {
        print_log("bind error");
        exit(1);
    }
    listen(sockfd,7);
}
int main(int argc, char** argv)
{
    init_listen();
    while (1) {
        int len = sizeof(cli);
        int socknew = accept(sockfd, (struct sockaddr*)&cli, (socklen_t*)&len);
        if (socknew < 0){
            print_log("accept error");
            continue;
        }
        char buff[1024] = { 0 }; //短连接不需要循环读取
        int n = recv(socknew, buff, 1023, 0);
        if (n <= 0) {
            close(socknew);
            print_log("recv error");
            continue;
        }
        printf("%s\n", buff); //将客户端HTTP请求内容输出
        // char pathname[128] = { 0 };
        // char filetype[10] = { 0 };
        // AnayRequest(buff, pathname, filetype); //分析参数
        // printf("%s\n", filetype);
        // SendData(pathname, c, filetype); //发送请求的html网页
        close(socknew);
    }
}