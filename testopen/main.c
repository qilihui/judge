#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
int main()
{
    printf("out 1\n");
    // system("pwd");
    chdir("../zdir");
    pid_t pid = fork();
    if (pid == 0) {
        char pwd_arr[100];
        getcwd(pwd_arr, sizeof(pwd_arr));
        printf("%s\n", pwd_arr);
        if (freopen("a.txt", "r", stdin) == NULL) {
            printf("error stdin\n");
            exit(3);
        }
        if (freopen("b.txt", "w", stdout) == NULL) {
            printf("error stdout\n");
            exit(3);
        }
        if (freopen("c.txt", "w", stderr) == NULL) {
            printf("error stdin\n");
            exit(3);
        }
        if (chroot(pwd_arr)) {
            printf("error  %s\n", strerror(errno));
        }
        char x[100];
        getcwd(x, sizeof(x));
        printf("%s\n", x);
        if (chdir("/home")) {
            printf("chdir 失败 %s\n", strerror(errno));
        }
        getcwd(x, sizeof(x));
        printf("%s\n", x);
        exit(0);
        if (execl("pa", "pa", NULL) == -1) {
            printf("execl error %s", strerror(errno));
        }
        printf("end\n");
    } else {
        printf("%d\n", pid);
        int status;
        waitpid(pid,&status,__WALL);
        printf("status = %d\n",status);
        printf("%d\n",WIFEXITED(status));
        printf("%d\n",WEXITSTATUS(status));

        printf("%d\n",WIFSIGNALED(status));
        printf("%d\n",WTERMSIG(status));
    }
}