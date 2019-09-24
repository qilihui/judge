#include<stdio.h>
#include<stdlib.h>
int x[1000000]={0};
int main()
{
    int sum=0;
    for(int i=0;i<1000000;i++){
        x[i]=i;
    }
    int a;
    scanf("%d",&a);
    // printf("执行problem\n");
    // printf("%d",sum);
    printf("%d",a+100);
}