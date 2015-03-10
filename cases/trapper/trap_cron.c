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

int check_fd_cron(msd_ae_event_loop *el, long long id, void *privdate) 
{
    msd_thread_worker_t *worker = (msd_thread_worker_t *)privdate;
    msd_dlist_iter_t dlist_iter;
    msd_dlist_node_t *node;
    back_end_t *back_end;
    trap_worker_data_t *worker_data = worker->priv_data;
    msd_dlist_t *dlist = worker_data->back_end_list;
    
    MSD_DEBUG_LOG("Worker[%d] check fd cron begin!", worker->idx);
   
    msd_dlist_rewind(dlist, &dlist_iter);
    while ((node = msd_dlist_next(&dlist_iter))) 
    {
        back_end = node->value; 
        msd_dlist_node_t *t_node = msd_dlist_index(back_end->address_list, back_end->idx);
        tos_addr_t       *addr = t_node->value; 
        
        MSD_DEBUG_LOG("Current fd:%d. IP:%s, Port:%d", back_end->fd, addr->back_ip, addr->back_port);
        if(MSD_OK != chose_one_avail_fd(back_end, worker))
        {
            MSD_DEBUG_LOG("Chose_one_avail_fd failed!");
        }
    }
    
    return 1000*worker_data->back_end_alive_interval;
}

/* 定期更新hostname=>game的映射关系 */
int update_host_game_cron(msd_ae_event_loop *el, long long id, void *privdate) 
{
    msd_thread_worker_t *worker = (msd_thread_worker_t *)privdate;
    trap_worker_data_t *worker_data = worker->priv_data;

    //TODO 这里面有个问题，对于删除的机器的hostname，无法发现并删除，此hash注定会越来越大
    //     但是不影响数据的正确性，暂不处理，因为相同的key会替换原有的val，并正确释放
    update_host_game_map(worker_data->host_game_map_intfs->buf, 
            worker_data->host_game_hash); 

    /*
    hash_dump(worker_data->host_game_hash);
    hash_dump2(worker_data->item_to_clu_item_hash);
    */
    return 1000*worker_data->host_game_interval;
}

/* 
 * 针对temp_hash表的foreach计算函数
 * 当前item_id，计算所对应所有host窗口的max,min,avg,sum 
 */
static int calculate_hash(const msd_hash_entry_t *he, void *userptr)
{
    cal_struct_t *cal = userptr;
    trap_worker_data_t *worker_data = cal->worker_data;
    char *item_id = he->key;
    msd_dlist_t *dlist = he->val;
    msd_dlist_iter_t dlist_iter;
    msd_dlist_t *cluster_item_list;
    msd_dlist_node_t *node;
    char *clu_item_id;
    float sum = 0;
    float max = FLT_MIN;
    float min = FLT_MAX;
    float avg = 0;
    int cnt = 0;
    char buf[128];
    back_end_t *back_end;
    cJSON *p_root;
    cJSON *p_data_arr;
    cJSON *p_data;
    cJSON *p_value;
    char  *json;
    char result[256] = {0};
    
    MSD_INFO_LOG("Item_id:%s",item_id);
    msd_dlist_rewind(dlist, &dlist_iter);
    while ((node = msd_dlist_next(&dlist_iter))) 
    {
        host_value_t *host_value = node->value;
        MSD_INFO_LOG("host:%s, value:%f",host_value->hostname, host_value->value);
        if(host_value->value > max)
        {
            max = host_value->value;
        }

        if(host_value->value < min)
        {
            min = host_value->value;
        }
        sum += host_value->value;
        cnt ++;
    }
    avg = sum/cnt;

    cnt = 0;
    cluster_item_list = msd_hash_get_val(worker_data->item_to_clu_item_hash, item_id);
    MSD_INFO_LOG("Cluster Name: cluster_%s", cal->game);


    /* 拼装JSON计算结果 */
    //0000000106{"type": "0", "host": "huaqi_3", "data": [["10001", "1", "1"], ["10002", "2", "1"]], "time": "1402972148"}
    p_root = cJSON_CreateObject();

    cJSON_AddStringToObject(p_root,"type", "0");
    memset(buf, 0, 128);
    snprintf(buf, 128, "cluster_%s", (char *)cal->game);
    cJSON_AddStringToObject(p_root,"host", buf);
    memset(buf, 0, 128);
    snprintf(buf, 128, "%d", (int)time(NULL));
    cJSON_AddStringToObject(p_root,"time", buf);
    
    msd_dlist_rewind(cluster_item_list, &dlist_iter);
    p_data_arr = cJSON_CreateArray();
    
    while ((node = msd_dlist_next(&dlist_iter))) 
    {
        clu_item_id = node->value;
        p_data = cJSON_CreateArray();
        switch (cnt)
        {
            case 0:
                memset(buf, 0, 128); snprintf(buf, 128, "%s", clu_item_id);
                p_value = cJSON_CreateString(buf);
                cJSON_AddItemToArray(p_data, p_value);

                memset(buf, 0, 128); snprintf(buf, 128, "%.2f", max);
                p_value = cJSON_CreateString(buf);
                cJSON_AddItemToArray(p_data, p_value);                

                p_value = cJSON_CreateString("1");
                cJSON_AddItemToArray(p_data, p_value); 

                MSD_INFO_LOG("MAX(item_id:%s):%f",clu_item_id, max);
                break;
            case 1:
                memset(buf, 0, 128); snprintf(buf, 128, "%s", clu_item_id);
                p_value = cJSON_CreateString(buf);
                cJSON_AddItemToArray(p_data, p_value);

                memset(buf, 0, 128); snprintf(buf, 128, "%.2f", min);
                p_value = cJSON_CreateString(buf);
                cJSON_AddItemToArray(p_data, p_value);                

                p_value = cJSON_CreateString("1");
                cJSON_AddItemToArray(p_data, p_value); 

                MSD_INFO_LOG("MIN(item_id:%s):%f",clu_item_id, min);
                break;
            case 2:
                memset(buf, 0, 128); snprintf(buf, 128, "%s", clu_item_id);
                p_value = cJSON_CreateString(buf);
                cJSON_AddItemToArray(p_data, p_value);

                memset(buf, 0, 128); snprintf(buf, 128, "%.2f", avg);
                p_value = cJSON_CreateString(buf);
                cJSON_AddItemToArray(p_data, p_value);                

                p_value = cJSON_CreateString("1");
                cJSON_AddItemToArray(p_data, p_value); 

                MSD_INFO_LOG("AVG(item_id:%s):%f",clu_item_id, avg);
                break;
            case 3:
                memset(buf, 0, 128); snprintf(buf, 128, "%s", clu_item_id);
                p_value = cJSON_CreateString(buf);
                cJSON_AddItemToArray(p_data, p_value);

                memset(buf, 0, 128); snprintf(buf, 128, "%.2f", sum);
                p_value = cJSON_CreateString(buf);
                cJSON_AddItemToArray(p_data, p_value);                

                p_value = cJSON_CreateString("1");
                cJSON_AddItemToArray(p_data, p_value); 
                
                MSD_INFO_LOG("SUM(item_id:%s):%f",clu_item_id, sum);
                break;                
                
        }
        cnt++;
        cJSON_AddItemToArray(p_data_arr, p_data); 
    }
    cJSON_AddItemToObject(p_root, "data", p_data_arr);
    json = cJSON_PrintUnformatted(p_root);

    snprintf(result, 256, "%010d%s", (int)strlen(json), json);
    MSD_INFO_LOG("JSON_RESULT:%s", result);
    
   /* 注册向后端写事件，将结果传送至后端 */
    dlist = worker_data->back_end_list;
	msd_dlist_rewind(dlist, &dlist_iter);
    while ((node = msd_dlist_next(&dlist_iter))) 
    {
        back_end = node->value;  
        // 信息写入sendbuf 
        msd_str_cat_len(&(back_end->sendbuf), result, strlen(result));

        if (back_end->status!=B_BAD 
            && back_end->fd != -1 
            && msd_ae_create_file_event(worker_data->worker->t_ael, back_end->fd, MSD_AE_WRITABLE,
                send_to_back, back_end) == MSD_ERR) 
        {
            MSD_ERROR_LOG("Create write file event failed for backend_end fd:%d", back_end->fd);
            return MSD_ERR;
        }
    }

    free(json);
    /* 递归删除，中间无需单独删除，否则反而会出现二次释放 */
    cJSON_Delete(p_root);
    
    return MSD_OK;
}

/*
 * 根据item_value_hash表中一个条目(一个业务)，计算单个业务的所有集群监控项的值
 */
static int handle_game_data(const msd_hash_entry_t *he, void *userptr)
{
    MSD_INFO_LOG("================= Game : %s Begin handle ===================", (char *)he->key);
    trap_worker_data_t *worker_data = userptr;
    msd_dlist_t *dlist = he->val;
    msd_dlist_iter_t dlist_iter;
    msd_dlist_node_t *node;
    item_value_t *item_value;
    msd_dlist_t *host_value_list;
    int cnt, i;
    cal_struct_t cal;

    /* 建立一个临时hash表，专门存储过滤之后的数据
      * key和item_to_clu_item_hash相同，一个item_id
      * value和item_value_hash相同，一个链表，每个node都是最新的值
      * 遍历此list即可获得该item_id对应的max,min,avg,sum
      */
    msd_hash_t *temp_hash;
    if(!(temp_hash = msd_hash_create(16)))
    {
        MSD_ERROR_LOG("Msd_hash_create Failed!");
        return MSD_ERR; 
    }
    MSD_HASH_SET_SET_KEY(temp_hash,  msd_hash_def_set);
    /* 没有set函数，表示需要在外部单独生成value */
    //MSD_HASH_SET_SET_VAL(temp_hash,  msd_hash_def_set);
    MSD_HASH_SET_FREE_KEY(temp_hash, msd_hash_def_free);
    MSD_HASH_SET_FREE_VAL(temp_hash, _hash_val_free_host_value);
    MSD_HASH_SET_KEYCMP(temp_hash,   msd_hash_def_cmp);

    /* 遍历item_value_hash的一个value(list),构造当前业务的临时hash */
    msd_dlist_rewind(dlist, &dlist_iter);
    while ((node = msd_dlist_next(&dlist_iter))) 
    {
        item_value = node->value;
        MSD_INFO_LOG("Host:%s, Value_str:%s, Time:%d", item_value->hostname, item_value->value_str->buf, item_value->time);
        unsigned char buf[2048] = {0};
        /* 假设有20个需要计算集群监控的监控项，时间窗口是3，则最多是20*3个分割 */
        unsigned char *i_v[60];
        strncpy((char *)buf, (const char *)item_value->value_str->buf, (size_t)2048);
        /* 切割value_str并按照item_id进行过滤去重 */
        cnt = msd_str_explode(buf, i_v, 60, (const unsigned char *)";");
        for(i=0; i<cnt; i++)
        {
            unsigned char *id_val[2];
            if(2 != msd_str_explode(i_v[i], id_val, 2, (const unsigned char *)","))
            {
                MSD_ERROR_LOG("Msd_str_explode failed:%s", i_v[i]);
            }
            if(!(host_value_list = msd_hash_get_val(temp_hash, id_val[0])))
            {
                /* 如果没有找到，则新增之 */
                if(!(host_value_list = msd_dlist_init()))
                {
                    MSD_ERROR_LOG("Msd_dlist_init failed!");
                    return MSD_ERR;
                }
                msd_dlist_set_free(host_value_list, _free_node_host_value);
                msd_hash_insert(temp_hash, id_val[0], host_value_list);
            }
            
            /* 根据host_name,找到item_value_list链表中对应的节点 */
            msd_dlist_node_t *value_node;
            if(!(value_node = host_value_list_search(host_value_list, item_value->hostname)))
            {
                host_value_t *host_value = calloc(1, sizeof(host_value_t));
                strncpy(host_value->hostname, item_value->hostname, 128);
                host_value->value = atof((const char *)id_val[1]);
                msd_dlist_add_node_tail(host_value_list, host_value);
            }
            else
            {
                host_value_t *host_value = value_node->value;
                strncpy(host_value->hostname, item_value->hostname, 128);
                host_value->value = atof((const char *)id_val[1]);
            }
        }
    }

    /* 遍历临时哈希表，在当前业务下面，逐个item_id进行统计汇总 */
    cal.game = he->key;
    cal.worker_data = worker_data;
    msd_hash_foreach(temp_hash, calculate_hash, &cal);

    /* 清空临时hash表 */
    msd_hash_free(temp_hash);

    // 清空item_value_hash对应本业务的这一支dlist. 移外层统一释放
    // msd_hash_remove_entry(worker_data->item_value_hash, he->key);
    
    return MSD_OK;
}

/* 时间事件，定期计算时间窗口，将计算结果向后端发送 */
int cal_send_back_cron(msd_ae_event_loop *el, long long id, void *privdate) 
{
    MSD_INFO_LOG("######################## BEGIN TO CALCULATE ##############################\n");
    msd_thread_worker_t *worker = (msd_thread_worker_t *)privdate;
    trap_worker_data_t *worker_data = worker->priv_data;

    msd_hash_t *item_value_hash = worker_data->item_value_hash;

    /* 遍历哈希，逐个业务进行计算! */
    msd_hash_foreach(item_value_hash, handle_game_data, worker_data);

    /* 计算完毕，统一清理全部哈希 */
    msd_hash_foreach(item_value_hash, _hash_delete_foreach, item_value_hash); /*将每个元素清除*/
    MSD_INFO_LOG("########################## END TO CALCULATE ##############################\n");
    return 1000*worker_data->cal_send_back_interval;
}


