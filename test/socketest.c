#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
//仿写http服务器,需要关闭httpd以及使用root用户操作

void SendError(int c) //发送错误页面（连带头部）
{
    char buff[1024] = { 0 };
    strcpy(buff, "HTTP/1.1 404 NOT FOUND\r\n");
    strcat(buff, "Server: myhttp/1.0\r\n");
    strcat(buff, "Content-Length: ");
    sprintf(buff + strlen(buff), "%d", 0);
    strcat(buff, "\r\n");
    strcat(buff, "Content-Type: text/html;charset=utf-8\r\n");
    strcat(buff, "\r\n"); //空行标识数据部分和头部分开
    strcat(buff, "404 NOT FOUND"); //发送给客户端的数据，用于显示
    send(c, buff, strlen(buff), 0);
}
void SendHead(int size, int c, char* filetype) //发送给客户端应答报头
{
    char buff[1024] = { 0 };
    strcpy(buff, "HTTP/1.1 200 OK\r\n");
    strcat(buff, "Server: myhttp/1.0\r\n");
    strcat(buff, "Content-Length: ");
    sprintf(buff + strlen(buff), "%d", size);
    strcat(buff, "\r\n");
    char filetype1[100];
    if (!strcmp("png", filetype)) {
        strcpy(filetype1, "image");
        sprintf(buff + strlen(buff), "Content-Type: %s/%s\r\n", filetype1, filetype);
    } else if (!strcmp("json", filetype)) {
        strcpy(filetype1, "application");
        sprintf(buff + strlen(buff), "Content-Type: %s/%s\r\n", filetype1, filetype);
    } else {
        strcpy(filetype1, "text");
        sprintf(buff + strlen(buff), "Content-Type: %s/%s;charset=utf-8\r\n", filetype1, filetype);
    }
    // sprintf(buff+strlen(buff),"Content-Type: %s/%s;charset=utf-8\r\n",filetype1,filetype);
    // strcat(buff,"Content-Type: text/html;charset=utf-8\r\n");
    strcat(buff, "\r\n"); //空行标识数据部分和头部分开
    send(c, buff, strlen(buff), 0);
}
void AnayRequest(char* buff, char* pathname, char* filetype) //解析参数获取到请求的页面地址
{
    //GET  /index.html HTTP/1.1   Linux上获取到的请求第一行，并没有域名及IP
    char* p = strtok(buff, " ");
    p = strtok(NULL, " "); //获取到了/index.html
    // printf("%s\n",p);
    // sleep(2);
    char pp[1000];
    if (!strcmp(p, "/")) {
        strcpy(pathname, "/home/tom/qduoj/index.html"); //请求的页面在本机的var/www/html底下
        strcpy(filetype, "html");
        return;
    }
    strcpy(pp, p);
    strtok(pp, ".");
    char* ppp = strtok(NULL, ".");
    strcpy(filetype, ppp);

    strcpy(pathname, "/home/tom/qduoj/oj/oj-前段"); //请求的页面在本机的var/www/html底下
    strcat(pathname, p); //将分割的页面名称与之相连接，得到请求页面的绝对地址

    printf("%s\n", p);
}
void SendData(char* pathname, int c, char* filetype) //发送页面
{
    struct stat st;
    if (-1 != stat(pathname, &st)) //通过路径获取文件属性
    {
        SendHead(st.st_size, c, filetype); //发送应答报头，传入请求页面文件的大小
        printf("******1\n");
    } else {
        SendError(c); //所有错误都发送这个404错误（只发送一句话而已）
        return;
    }
    int fd = open(pathname, O_RDONLY); //发送页面正文给客户端进行显示，即请求的页面
    if (fd == -1) {
        SendError(c);
        return;
    }
    while (1) {
        char buff[128] = { 0 };
        int n = read(fd, buff, 127);
        if (n <= 0) {
            close(fd);
            break;
        }
        send(c, buff, strlen(buff), 0); //发送页面内容给客户端
    }
}

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd != -1);

    struct sockaddr_in ser, cli;
    ser.sin_family = AF_INET;
    ser.sin_port = htons(80);
    ser.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(sockfd, (struct sockaddr*)&ser, sizeof(ser))) {
        printf("bind error\n");
        exit(0);
    }
    listen(sockfd, 5);
	
    while (1) {
        int len = sizeof(cli);
        int c = accept(sockfd, (struct sockaddr*)&cli, &len);
        if (c < 0)
            continue;
        char buff[1024] = { 0 }; //短连接不需要循环读取
        int n = recv(c, buff, 1023, 0);
        if (n <= 0) {
            close(c);
            continue;
        }
        printf("%s\n", buff); //将客户端HTTP请求内容输出
        char pathname[128] = { 0 };
        char filetype[10] = { 0 };
        AnayRequest(buff, pathname, filetype); //分析参数
        printf("%s\n", filetype);
        SendData(pathname, c, filetype); //发送请求的html网页
        close(c);
    }
}