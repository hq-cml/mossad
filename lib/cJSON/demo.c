//cJson demos
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
static int recursive_parse(cJSON *p_root, char *prefix);
int recursive_parse(cJSON *p_root, char *prefix)
{
    int i;
    int data_count;
    cJSON *p_item;
    
    //拼接前面的空格
    char pre[100] = {0};
    strcpy(pre, prefix);
    strcat(pre, "    ");
    
    
    int type = p_root->type;
    switch(type)
    {
        case cJSON_False:
        case cJSON_NULL:
        case cJSON_True:
            return -1;
            break;
            
        case cJSON_Number:
            if(p_root->string)
                printf("%s%s:%d(%f)\n", pre, p_root->string, p_root->valueint, p_root->valuedouble);
            else
                printf("%s%d(%f)\n", pre, p_root->valueint, p_root->valuedouble);
            break;            
        case cJSON_String:
            if(p_root->string)
                printf("%s%s:%s\n", pre, p_root->string, p_root->valuestring);
            else
                printf("%s%s\n", pre, p_root->valuestring);
            break;
            
        case cJSON_Array:
        case cJSON_Object:
            data_count = cJSON_GetArraySize(p_root);
            for(i=0; i<data_count; i++)
            {
                p_item = cJSON_GetArrayItem(p_root, i);
                if(!p_item)
                    return -1;
                recursive_parse(p_item, pre);
            }
            break;            
    }
    return 0;
}
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
