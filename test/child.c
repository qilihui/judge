#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
int main(int argc,char **argv)
{
    sleep(3);
    FILE* fp = fopen("./child.txt","w");
    fprintf(fp,"%d\n%s",argc,argv[1]);
    fclose(fp);
}