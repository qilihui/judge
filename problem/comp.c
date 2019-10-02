#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
int main()
{
    const char *CP_C[] = {"gcc", "main.c", "-o", "main", "-O2", "-fmax-errors=10", "-Wall",
						  "-lm", "--static", "-std=c99", "-DONLINE_JUDGE", NULL};
    execvp(CP_C[0], (char *const *)CP_C);
    system("rm /home/tom/work/problem/testfile");
    printf("\n234234\n");
}