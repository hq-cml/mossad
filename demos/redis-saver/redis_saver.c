/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  redis_saver.c
 * 
 * Description :  接收数组存储于Redis
 * 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/

#include "redis_saver.h"

static int redis_connect(void *worker_data, redisContext **c);
static int redis_save(redisContext *c, const char* hostname, const char* item_id, const char* value);
static int redis_destroy(redisContext *c);


/**
 * 功能: 连接redis
 * 参数: @data:woker私有数据
 * 返回:成功:0, 失败:-x
 **/
int redis_connect(void *data, redisContext **c)
{
    int port;
    int db;
    redisReply* r=NULL;
    char cmd[100] = {0};
    
    saver_worker_data_t *worker_data = (saver_worker_data_t *)data;
    port = atoi(worker_data->redis_port->buf);
    db   = atoi(worker_data->redis_db->buf);
    
    //连接Redis服务器，同时获取与Redis连接的上下文对象。    
    //该对象将用于其后所有与Redis操作的函数。    
    *c = redisConnect(worker_data->redis_ip->buf, port);    
    if (c->err) {
        MSD_ERROR_LOG("Connect error: %s", c->errstr);         
        //redisFree(c);        
        return MSD_FAILED;    
    }

    //选择库
    snprintf(cmd, 100, "select %d", db);    
    r = (redisReply*)redisCommand(c, cmd);    
    if (r == NULL) {        
        MSD_ERROR_LOG("Select error: %s", c->errstr); 
        //redisFree(c);        
        return;    
    }    

    //由于后面重复使用该变量，所以需要提前释放，否则内存泄漏。    
    freeReplyObject(r);

    return MSD_OK;
}
/**
 * 功能: redis的存储函数
 * 参数: @conf
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int redis_save(redisContext *c, const char* hostname, const char* item_id, const char* value)
{
    redisReply* r=NULL;
    char cmd[4096] = {0};

    snprint(cmd, 4096, "hset %s %s %s", hostname, item_id, value);    
    r = (redisReply*)redisCommand(c,cmd);        
    if (NULL == r) {        
        MSD_ERROR_LOG("Hset Error: %s\n", c->errstr);        
        //redisFree(c);        
        return MSD_FAILED;    
    }    

    if (!(strcasecmp(r->str,"OK") == 0)) {        
        MSD_ERROR_LOG("Error: %s\n", c->errstr);        
        MSD_ERROR_LOG("Failed to execute command[%s]",cmd);        
        freeReplyObject(r);        
        //redisFree(c);        
        return MSD_FAILED;    
    }


    return MSD_OK;
}
/**
 * 功能: 单个线程初始化回调
 * 参数: @worker
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int msd_handle_worker_init(void *conf, void *arg)
{
    MSD_INFO_LOG("Msd_handle_worker_init is called!");
    msd_thread_worker_t *worker = (msd_thread_worker_t *)arg;

    saver_worker_data_t *worker_data;
    
    if(!(worker->priv_data = calloc(1, sizeof(saver_worker_data_t))))
    {
        MSD_ERROR_LOG("Calloc priv_data failed!");
        return MSD_ERR;
    }
    worker_data = worker->priv_data;
    if(!(worker_data->redis_ip= msd_str_new(msd_conf_get_str_value(conf,"redis_ip",NULL))))
    {
        MSD_ERROR_LOG("Get redis_ip Failed!");
        return MSD_ERR;
    }
    if(!(worker_data->redis_port= msd_str_new(msd_conf_get_str_value(conf,"redis_port",NULL))))
    {
        MSD_ERROR_LOG("Get redis_port Failed!");
        return MSD_ERR;
    }
    if(!(worker_data->redis_db= msd_str_new(msd_conf_get_str_value(conf,"redis_db",NULL))))
    {
        MSD_ERROR_LOG("Get redis_db Failed!");
        return MSD_ERR;
    }
    
    worker_data->worker = worker;
 
    return MSD_OK;
}
