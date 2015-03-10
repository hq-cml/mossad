/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  trapper.c
 * 
 * Description :  trapper
 * 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/

#include "trapper.h"

static size_t host_game_callback(char * ptr, size_t size, size_t nmemb, void * userdata);

int update_host_game_map(char *url, msd_hash_t *hash)
{
	CURLcode res;
	CURL *curl = curl_easy_init();

	if (curl == NULL)
		return MSD_ERR;

	curl_easy_setopt(curl, CURLOPT_URL, url); 
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2); //设置超时时间, 2秒
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, host_game_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, hash);
	
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) 
    {
		return MSD_ERR;
	}

	curl_easy_cleanup(curl);

    return MSD_OK;
}

static size_t host_game_callback(char * content, size_t size, size_t nmemb, void * userdata)
{
    msd_hash_t *host_game_hash = userdata;
	int i;
	char * begin, * start, *ptr;
	char buff[10240];
	char key[10240];  

	memset(buff, '\0', sizeof(buff));

	for (i = 0, begin = content, start = buff; i < size * nmemb; i++, begin++)
	{
	    /* 取一行 */
		if (*begin == '\n') 
        {
			*start = '\0';
			ptr = strchr(buff, '#');

			if(ptr != NULL) 
            {
				*ptr = '\0';   
                snprintf(key, sizeof(key), "%s", buff);
                msd_hash_insert(host_game_hash, key, ptr+1);
			}

			memset(buff, '\0', sizeof(buff));
			memset(key, '\0', sizeof(key));
			
			start = buff;
			continue;
			
		}
        else 
        {
			*start = *begin;
			start++;
		}
	}
	return size * nmemb;
}

/*
 * 功能: 处理配置文件中item_id_list块的一行，将此行数据的内容,分割开，变成一个链表，作为item_id的映射值
 */
int _hash_deal_with_item_foreach(const msd_hash_entry_t *he, void *userptr)
{
    msd_conf_value_t *cv = he->val;
    trap_worker_data_t *worker_data = userptr;
    //MSD_INFO_LOG("CON:%s===>%s\n", he->key, ((msd_conf_entry_t*)cv->value)->value);
    msd_hash_t *hash = worker_data->item_to_clu_item_hash;

    //1001   5001,5002,5003,5004
    //he->key : 配置文件中的一行item_id(1001)；
    //(msd_conf_entry_t*)cv->value)->value : 配置文件中的一行的值(5001,5002,5003,5004)
    msd_hash_insert(hash, he->key, ((msd_conf_entry_t*)cv->value)->value);
    
    return MSD_OK;
}

/**
 * 功能: 哈希表set value函数
 **/
void *_hash_val_set_item_id(const void *item_id_list)
{    
    unsigned char buf[1024] = {0};
    msd_dlist_t *dlist;
    memset(buf, 0, 1024);
    unsigned char *item_id[4];
    int cnt, i;
    strcpy((char *)buf, (const char *)item_id_list);


    /* 集群监控项的id链表 */
    if(!(dlist = msd_dlist_init()))
    {
        MSD_ERROR_LOG("Msd_dlist_init failed!");
        return NULL;
    }
    msd_dlist_set_free(dlist, free);

    if((cnt = msd_str_explode(buf, item_id, 4, (const unsigned char *)","))!=4)
    {
        MSD_ERROR_LOG("Msd_str_explode failed!");
        return NULL;        
    }

    //循环初始化back_end_list
    for (i=0; i<cnt; i++)
    {
       char *str = strdup((const char *)item_id[i]);
       msd_dlist_add_node_tail(dlist, str);
    }

    return dlist;
}

/**
 * 功能: 哈希表free value函数
 **/
void _hash_val_free_item_id(void *dlist)
{    
    msd_dlist_destroy(dlist);
    return;
}

/**
 * 功能: 哈希表free value函数
 **/
void _hash_val_free_item_value(void *dlist)
{
    msd_dlist_destroy(dlist);
    return;
}

/**
 * 功能: 哈希表free value函数
 **/
void _hash_val_free_host_value(void *dlist)
{
    msd_dlist_destroy(dlist);
    return;
}

/*
 * 功能: item_value_list 寻找函数
 **/
msd_dlist_node_t *item_value_list_search(msd_dlist_t *dl, char *hostname) 
{
    msd_dlist_iter_t *iter = NULL;
    msd_dlist_node_t *node = NULL;
    iter = msd_dlist_get_iterator(dl, MSD_DLIST_START_HEAD);
    while ((node = msd_dlist_next(iter)) != NULL) 
    {
        item_value_t *value = node->value;
        if (!strcmp(value->hostname, hostname)) 
        {
            msd_dlist_destroy_iterator(iter);
            return node;
        }
    }
    msd_dlist_destroy_iterator(iter);
    return NULL;
}

/*
 * 功能: item_value_list 寻找函数
 **/
msd_dlist_node_t *host_value_list_search(msd_dlist_t *dl, char *hostname) 
{
    msd_dlist_iter_t *iter = NULL;
    msd_dlist_node_t *node = NULL;
    iter = msd_dlist_get_iterator(dl, MSD_DLIST_START_HEAD);
    while ((node = msd_dlist_next(iter)) != NULL) 
    {
        host_value_t *value = node->value;
        if (!strcmp(value->hostname, hostname)) 
        {
            msd_dlist_destroy_iterator(iter);
            return node;
        }
    }
    msd_dlist_destroy_iterator(iter);
    return NULL;
}



/*
 * node生成函数
 */
item_value_t *item_value_node_init(char *hostname) 
{
    item_value_t *v;

    if ((v= malloc(sizeof(*v))) == NULL) 
    {
        return NULL;
    }
    memset(v->hostname, 0, 128);
    strncpy(v->hostname, hostname, 128);

    if(!(v->value_str = msd_str_new_empty()))
    {
        free(v);
        return NULL;
    }
    v->time = time(NULL);
    
    return v;
}

/*
 * node释放函数
 */
void _free_node_item_value(void *ptr)
{
    item_value_t *v = ptr;
    msd_str_free(v->value_str);
}

/*
 * node释放函数
 */
void _free_node_host_value(void *ptr)
{
    free(ptr);
}

/* 
 * 哈希删除节点函数
 */
int _hash_delete_foreach(const msd_hash_entry_t *he, void *ht)
{
    if(msd_hash_remove_entry(ht, he->key) != MSD_OK)
    {
        return MSD_ERR;
    }

    return MSD_OK;
}

