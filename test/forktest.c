#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
int main()
{
    char* a = (char*)malloc(sizeof(char) * 100);
    for (int i = 0; i < 99; i++) {
        a[i] = i + '0';
    }
    a[99] = 0;
    const char* p = a;
    printf("%s\n", p);
    // pid_t pid = fork();
    // if (pid == 0) {
    //     execlp("./child","child",a,NULL);
    // } else if (pid > 0) {
    free(a);
    printf("%s\n", a);
    printf("%s\n", p);
    // } else {
    //     printf("fork fail\n");
    // }
}