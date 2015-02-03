//cJson demos
#include <stdio.h>
#include <math.h>

int json_decode()
{
    int i;
    cJSON *p_root;
    cJSON *p_item1;
    cJSON *p_item2;
    cJSON *p_item3;
    cJSON *p_array;

    char *p_item_id;
    char *p_value;
    char *content_buff;
    
    content_buff = calloc(1, 4096);
    strcpy(content_buff, str);

    /* 苦逼的json解析，只需要cJSON_Parse的返回对象需要释放，内部会循环释放 */
    p_root = cJSON_Parse(content_buff+10);
    if(!p_root) goto json_null;
    
    return 0;
}

char* json_encode()
{
    return 0; 
}

int main()
{
    
    return 0;
}
