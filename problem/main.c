#include <stdio.h>
#include <math.h>
int main(){
    int p=0;
    for(int i=0;i<100000;i++)
        for(int j=0;j<100000;j++)
            p++;
    int a, b;
    scanf("%d", &a);
    printf("%d\n", sqrt(a));
    return 0;
}