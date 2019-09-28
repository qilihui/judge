#include "cJSON.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void print_log(const char* s)
{
    printf("%s: %s\n", s, strerror(errno));
}
int main(int argc, char** argv)
{
    int serv_sockfd, clnt_sockfd;
    struct sockaddr_in serv_addr, clnt_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    if (setsockopt(serv_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        print_log("setsockopet error");
        exit(1);
    }
    if (bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in)) < 0) {
        print_log("bind error");
        exit(1);
    }
    if (listen(serv_sockfd, 3) < 0) {
        print_log("listen error");
        exit(1);
    }

    socklen_t len_t = sizeof(struct sockaddr_in);
    clnt_sockfd = accept(serv_sockfd, (struct sockaddr*)&clnt_addr, &len_t);
    if (clnt_sockfd < 0) {
        print_log("bind error");
        exit(0);
    }
    // close(serv_sockfd);
    printf("有客户端连接 IP:%s Port:%d\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));

    char recv_buffer[8192];
    int x = 10;
    while (x--) {
        memset(recv_buffer, 0, 8192);
        ssize_t recv_len = recv(clnt_sockfd, recv_buffer, sizeof(recv_buffer), 0);
        if (recv_len == 0) {
            printf("通信结束\n");
            break;
        } else if (recv_len < 0) {
            printf("读取数据出错\n");
            break;
        } else {
            printf("recv: %s\n", recv_buffer);
        }
        const char *p="\r\n\r\n";
        strtok(recv_buffer,p);
        char *s=strtok(NULL,p);
        printf("***************\n%s\n******************\n",s);
        char buff[1024] = { 0 };
        strcpy(buff, "HTTP/1.1 200 OK\r\n");
        strcat(buff, "Server: myhttp/1.0\r\n");
        strcat(buff, "Content-Length: ");
        sprintf(buff + strlen(buff), "%d", 3);
        strcat(buff, "\r\n");
        strcat(buff, "Content-Type: text/html;charset=utf-8\r\n");
        strcat(buff, "\r\n");
        strcat(buff, "121");
        send(clnt_sockfd, buff, strlen(buff), 0);
    }
    printf("服务器关闭\n");
    close(clnt_sockfd);
    close(serv_sockfd);
    return 1;
}