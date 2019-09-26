#include<stdio.h>
#include"cJSON.h"
int main()
{
    char *s="{\"name\": \"qilihui\",\"mima\": \"hahahah\",\"num\":  1}";
    cJSON *json1=cJSON_Parse(s);
    cJSON *name=cJSON_GetObjectItem(json1,"name");
    printf("%s",name->valuestring);
    cJSON_Delete(json1);
}