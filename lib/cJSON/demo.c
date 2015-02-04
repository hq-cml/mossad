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
    
    //拼接前缀的空格
    char pre[100] = {0};
    strcpy(pre, prefix);
    strcat(pre, "    ");
    
    /*
    //若明确知晓key对应value，则:
    //string、int、double
    cJSON *p_item1 = cJSON_GetObjectItem(p_root, "key");
    value = p_item1->valuestring/valueint/valuedouble;
    //object、array
    data_count = cJSON_GetArraySize(p_root);
    for(i=0; i<data_count; i++){
        p_item = cJSON_GetArrayItem(p_root, i);
        ...
    }
    */
    
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
                printf("%s%s => %d(%f)\n", pre, p_root->string, p_root->valueint, p_root->valuedouble);
            else
                printf("%s%d(%f)\n", pre, p_root->valueint, p_root->valuedouble);
            break;            
        case cJSON_String:
            if(p_root->string)
                printf("%s%s => %s\n", pre, p_root->string, p_root->valuestring);
            else
                printf("%s%s\n", pre, p_root->valuestring);
            break;
            
        case cJSON_Array:
            if(p_root->string)
                printf("%s%s => [\n", pre, p_root->string);
            else
                printf("%s[\n", pre);
            data_count = cJSON_GetArraySize(p_root);
            for(i=0; i<data_count; i++)
            {
                p_item = cJSON_GetArrayItem(p_root, i);
                if(!p_item)
                    return -1;
                recursive_parse(p_item, pre);
            }
            printf("%s]\n", pre);
            
            break; 
        case cJSON_Object:
            if(p_root->string)
                printf("%s%s=>{\n", pre, p_root->string);
            else
                printf("%s{\n", pre);
            data_count = cJSON_GetArraySize(p_root);
            data_count = cJSON_GetArraySize(p_root);
            for(i=0; i<data_count; i++)
            {
                p_item = cJSON_GetArrayItem(p_root, i);
                if(!p_item)
                    return -1;
                recursive_parse(p_item, pre);
            }
            printf("%s}\n", pre);
           
            break;            
    }
    return 0;
}

int json_decode(const char *str)
{
    cJSON *p_root;
    char *content_buff;
    char pre[100] = {0};
    
    content_buff = calloc(1, 4096);
    strcpy(content_buff, str);
    
    printf("\njson decode:\n");
    /* 苦逼的json解析，只需要cJSON_Parse的返回对象需要释放，内部会循环释放 */
    p_root = cJSON_Parse(content_buff);
    if(!p_root) goto json_null;
    
    if(-1 == recursive_parse(p_root, pre)){
        goto json_null;
    }

    free(content_buff);
    cJSON_Delete(p_root);
    return 0;
    
json_null:
    printf("Something is wrong!\n");
    free(content_buff);
    cJSON_Delete(p_root);
    return -1;

}

char* json_encode()
{
    cJSON *p_root;
    cJSON *p_data_arr;
    cJSON *p_data;
    cJSON *p_value;
    char  *result,*json;
    json = calloc(1, 4096);

    /* 拼装JSON计算结果 */
    //{"type":"0","abc":888.888000,"data":[["abc","efg","789.111"],[123,456,789]],"arr0":["xyz",888],"arr1":{"a":"uuu","b":111}}
    p_root = cJSON_CreateObject();
    
    //新增普通string和number
    cJSON_AddStringToObject(p_root, "type", "0");
    cJSON_AddNumberToObject(p_root, "abc",  888.888);
    
    //增加多为数组
    p_data_arr = cJSON_CreateArray();
    p_data = cJSON_CreateArray();
    p_value = cJSON_CreateString("abc");
    cJSON_AddItemToArray(p_data, p_value);
    p_value = cJSON_CreateString("efg");
    cJSON_AddItemToArray(p_data, p_value);
    p_value = cJSON_CreateString("789.111");
    cJSON_AddItemToArray(p_data, p_value);
    cJSON_AddItemToArray(p_data_arr, p_data); 
    
    p_data  = cJSON_CreateArray();
    p_value = cJSON_CreateNumber(123.0);
    cJSON_AddItemToArray(p_data, p_value);
    p_value = cJSON_CreateNumber(456.0);
    cJSON_AddItemToArray(p_data, p_value);
    p_value = cJSON_CreateNumber(789.0);
    cJSON_AddItemToArray(p_data, p_value);
    cJSON_AddItemToArray(p_data_arr, p_data); 
    
    cJSON_AddItemToObject(p_root, "data", p_data_arr);
    
    //增加一维数组
    p_data_arr = cJSON_CreateArray();
    p_value = cJSON_CreateString("xyz");
    cJSON_AddItemToArray(p_data_arr, p_value);
    p_value = cJSON_CreateNumber(888);
    cJSON_AddItemToArray(p_data_arr, p_value);
    cJSON_AddItemToObject(p_root, "arr0", p_data_arr); 

    //增加hash
    p_data_arr = cJSON_CreateObject();
    cJSON_AddStringToObject(p_data_arr, "a", "uuu");
    cJSON_AddNumberToObject(p_data_arr, "b", 111);
    cJSON_AddItemToObject(p_root, "arr1", p_data_arr);     
    
    //生成格式化字符串
    result = cJSON_PrintUnformatted(p_root);
    
    printf("orgin string: %s\n", result);
    strcpy(json, result);
    free(result);
    
    /* 递归删除，中间无需单独删除，否则反而会出现二次释放 */
    cJSON_Delete(p_root);
    return json; 
}

int main()
{
    //char *str = "{\"abc\":\"aaa\",\"bbb\":\"ccc\",\"arr0\":[123,456,789.888],\"arr1\":{\"a\":\"aa\",\"b\":9111}}";
    char *str = json_encode();
    json_decode(str);
	free(str);
    return 0;
}
