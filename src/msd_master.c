/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_master.c
 * 
 * Description :  Msd_master. Mossad master thread work.
 * 
 *     Created :  Apr 22, 2012 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/
#include "msd_core.h"

extern msd_instance_t *g_ins;

static void msd_master_accept(msd_ae_event_loop *el, int fd, 
        void *client_data, int mask);
static int msd_create_client(int cli_fd, const char *cli_ip, 
        int cli_port);
static int msd_client_find_free_slot();
static int msd_client_clear(msd_conn_client_t *client);
static int msd_thread_worker_dispatch(int client_idx);
static int msd_thread_list_find_next();
static int msd_master_cron(msd_ae_event_loop *el, long long id, void *privdate);
static void msd_master_recv_signal(msd_ae_event_loop *el, int fd, 
        void *client_data, int mask);
static void msd_shut_down();

/**
 * 功能: 主线程工作
 * 参数: 
 * 描述:
 *      1. client_vec的元素师conn_client的指针!而不是conn_client本身
 * 返回:成功:则会一直阻塞在ae_main里面，不返回; 失败:-x
 **/
int msd_master_cycle() 
{
    int  listen_fd;
    char error_buf[MSD_ANET_ERR_LEN];
    
    memset(error_buf, 0, MSD_ANET_ERR_LEN);
    msd_master_t *master = calloc(1, sizeof(msd_master_t));
    if(!master)
    {
        MSD_ERROR_LOG("Create master failed");
        return MSD_ERR;
    }
    g_ins->master          = master;
    master->start_time     = time(NULL);
    master->cur_conn       = -1;
    master->cur_thread     = -1;
    master->total_clients  = 0;
    master->client_limit   = msd_conf_get_int_value(g_ins->conf, "client_limit", 100000);
    master->poll_interval  = msd_conf_get_int_value(g_ins->conf, "master_poll_interval", 600);
    master->m_ael          = msd_ae_create_event_loop();
    if(!master->m_ael)
    {
        MSD_ERROR_LOG("Create AE Pool failed");
        free(master);
        return MSD_ERR;
    }
    
    master->client_vec     = msd_vector_new(master->client_limit, sizeof(msd_conn_client_t *));
    if(!master->client_vec)
    {
        MSD_ERROR_LOG("Create AE Pool failed");
        msd_ae_free_event_loop(master->m_ael);
        free(master);
        return MSD_ERR;    
    }
    
    /* 创建服务器 */
    listen_fd = msd_anet_tcp_server(error_buf, 
        msd_conf_get_str_value(g_ins->conf, "host", NULL), /* NULL表示bind所有网卡 */
        msd_conf_get_int_value(g_ins->conf, "port", 9527)
    );
    if (listen_fd == MSD_ERR) 
    {
        MSD_ERROR_LOG("Create Server failed");
        msd_ae_free_event_loop(master->m_ael);
        free(master);
        return MSD_ERR;
    }

    MSD_INFO_LOG("Create Server Success, listen_fd:%d", listen_fd);
    
    /* 设置非阻塞和nodelay */
    if((MSD_OK != msd_anet_nonblock(error_buf, listen_fd)) 
        || (MSD_OK != msd_anet_tcp_nodelay(error_buf, listen_fd)))
    {
        MSD_ERROR_LOG("Set noblock or nodelay failed");
        msd_ae_free_event_loop(master->m_ael);
        close(listen_fd);
        free(master);
        return MSD_ERR;        
    }
    MSD_DEBUG_LOG("Set Nonblock and Nodelay Success");
    master->listen_fd = listen_fd;

    /* 注册listen_fd的读取事件 */
    if (msd_ae_create_file_event(master->m_ael, listen_fd, 
                MSD_AE_READABLE, msd_master_accept, NULL) == MSD_ERR) 
    {
        MSD_ERROR_LOG("Create file event failed");
        msd_ae_free_event_loop(master->m_ael);
        close(listen_fd);
        free(master);
        return MSD_ERR; 
    }

    /* 注册sig_worker->notify_read_fd的读取事件 */
    if (msd_ae_create_file_event(master->m_ael, g_ins->sig_worker->notify_read_fd, 
                MSD_AE_READABLE, msd_master_recv_signal, NULL) == MSD_ERR) 
    {
        MSD_ERROR_LOG("Create file event failed");
        msd_ae_free_event_loop(master->m_ael);
        close(listen_fd);
        free(master);
        return MSD_ERR; 
    }

    /* 注册master时间事件 */
    if(msd_ae_create_time_event(master->m_ael, master->poll_interval*1000, 
            msd_master_cron, master, NULL) == MSD_ERR)
    {
        MSD_ERROR_LOG("Create time event failed");
        msd_ae_free_event_loop(master->m_ael);
        close(listen_fd);
        free(master);
        return MSD_ERR; 
    }

    MSD_INFO_LOG("Create Master Ae Success");
    msd_ae_main_loop(master->m_ael);
    return MSD_OK;
}

/**
 * 功能: 销毁master资源
 * 参数: 
 * 描述:
 *      1. 
 **/
int msd_master_destroy()
{
    //TODO
    return 0;
}
 
/**
 * 功能: 主线程accept操作
 * 参数: @el, ae句柄 
 *       @fd, 需要accept的listen_fd 
 *       @client_data, 额外参数
 *       @mask, 需要处理的事件 
 * 描述:
 *      1. 此函数是回调函数，所有的参数都是是由ae_main调用
 *         process_event函数的时候赋予的
 **/
static void msd_master_accept(msd_ae_event_loop *el, int fd, 
        void *client_data, int mask)
{
    int cli_fd, cli_port;
    char cli_ip[16];
    char error_buf[MSD_ANET_ERR_LEN];
    int worker_id;
    int client_idx;
    
    MSD_AE_NOTUSED(el);
    MSD_AE_NOTUSED(mask);
    MSD_AE_NOTUSED(client_data);

    MSD_DEBUG_LOG("Master Begin Accept");
    /* 获得client的fd，和其ip和port */   
    cli_fd = msd_anet_tcp_accept(error_buf, fd, cli_ip, &cli_port);
    if (cli_fd == MSD_ERR) 
    {
        MSD_ERROR_LOG("Accept failed:%s", error_buf);
        return;
    }

    MSD_INFO_LOG("Receive connection from %s:%d.The client_fd is %d.", cli_ip, cli_port, cli_fd);

    /* 设置非阻塞和nodelay */
    if((MSD_OK != msd_anet_nonblock(error_buf, cli_fd)) 
        || (MSD_OK != msd_anet_tcp_nodelay(error_buf, cli_fd)))
    {
        close(cli_fd);
        MSD_ERROR_LOG("Set client_fd noblock or nodelay failed");
        return;        
    }    

    /* 创建client结构 */
    if(MSD_ERR == (client_idx = msd_create_client(cli_fd, cli_ip, cli_port)))
    {
        close(cli_fd);
        MSD_ERROR_LOG("Create client failed, client_idx:%d", client_idx);
        return;         
    }

    /* 任务下发 */
    if(MSD_ERR == (worker_id = msd_thread_worker_dispatch(client_idx)))
    {
        msd_close_client(client_idx, "Dispatch failed!");
        MSD_ERROR_LOG("Worker dispatch failed, worker_id:%d", worker_id);
    }

    MSD_INFO_LOG("Worker[%d] Get the client[%d] task. Ip:%s, Port:%d", worker_id, client_idx, cli_ip, cli_port);
    return;
}

/**
 * 功能: 主线程创建client操作
 * 参数: @cli_fd，client对应的fd
 *       @cli_ip，client IP
 *       @cli_port,client Port
 * 描述:
 *      1. cliet->magic，初始被指为魔幻数，用户后续检查是否非法更改
 *      2. msd_vector_set_at的第三个参数是元素的指针，而conn_vec的元素是指针类型,
 *         所以msd_vector_set_at， 第三个参数应该是个二级指针 
 * 返回:成功:新生成client的idx，不返回; 失败:-x 
 **/
static int msd_create_client(int cli_fd, const char *cli_ip, int cli_port)
{
    int idx;
    msd_master_t *master = g_ins->master;
    msd_conn_client_t **pclient;
    msd_conn_client_t  *client;
    
    idx = msd_client_find_free_slot();
    if(MSD_ERR == idx)
    {
        MSD_ERROR_LOG("Max client num. Can not create more");

        /* 尝试写回失败原因，cli_fd非阻塞 */
        write(cli_fd, "Max client num.\n", strlen("Max client num.\n"));
        return MSD_ERR;
    }

    MSD_LOCK_LOCK(g_ins->client_conn_vec_lock);
    pclient = (msd_conn_client_t **)msd_vector_get_at(master->client_vec, idx);
    client  = *pclient;
    if (!client)
    {
        /* client有可能为空，如果曾经没有放置过client，就会是空
         * 如果曾经的client被close了，则不是空
         */
        client = (msd_conn_client_t *)calloc(1, sizeof(*client));
        if (!client) 
        {
            MSD_ERROR_LOG("Create client connection structure failed");
            MSD_LOCK_UNLOCK(g_ins->client_conn_vec_lock);
            return MSD_ERR;
        }
        msd_vector_set_at(master->client_vec, idx, (void *)&client);
        MSD_DEBUG_LOG("Create client struct.Idx:%d", idx);
    }

    /* client总数自增 */
    master->total_clients++;
    MSD_LOCK_UNLOCK(g_ins->client_conn_vec_lock);
    
    msd_client_clear(client);
    /* 初始化cli结构 */
    client->magic = MSD_MAGIC_DEBUG;  /* 初始化魔幻数 */
    client->fd = cli_fd;
    client->close_conn = 0;
    client->recv_prot_len = 0;
    client->remote_ip = strdup(cli_ip);
    client->remote_port = cli_port;
    client->recvbuf = msd_str_new_empty(); 
    client->sendbuf = msd_str_new_empty(); 
    client->access_time = time(NULL);    
    client->idx = idx;

    return idx;

}


/**
 * 功能: 在master线程的conn_vec中找到一个空闲的位置
 * 描述:
 *      1. 遍历是从last_conn开始的，这样可以避免每次都从头开始，提高效率
 *         但是也会造成中间出现孔洞(某些client关闭了连接)，利用mod操作，
 *         实现循环，填补孔洞
 *      2. 找到client的条件，如果client为NULL，说明曾经没有初始化，如果
 *         client->access_time为空，说明是孔洞，曾经的client已关闭
 * 返回:成功:0，不返回; 失败:-x 
 **/
static int msd_client_find_free_slot()
{
    int i;
    int idx = -1;
    msd_master_t *master = g_ins->master;
    msd_conn_client_t **pclient;
    msd_conn_client_t *client;
    
    MSD_LOCK_LOCK(g_ins->client_conn_vec_lock);
    for (i = 0; i < master->client_limit; i++)
    {
        idx = (i + master->cur_conn+ 1) % master->client_limit;
        pclient = (msd_conn_client_t **)msd_vector_get_at(master->client_vec, idx);
        client = *pclient;
        if (!client || 0 == client->access_time)
        {
            master->cur_conn = idx;
            break;
        }
    }
    MSD_LOCK_UNLOCK(g_ins->client_conn_vec_lock);

    if (i == master->client_limit)
        idx = -1;

    return idx;
}

/**
 * 功能: 清空conn_client结构
 * 描述:
 *      1. 只是清空client结构里面的成员，client本身不释放
 *      2. 对于是指针的成员，需要放置二次free
 * 返回:成功:0，不返回; 失败:-x 
 **/
static int msd_client_clear(msd_conn_client_t *client)
{
    assert(client);
    if (client)
    {
        client->fd = -1;
        client->close_conn= 0;
        if(client->remote_ip)
        {
            free(client->remote_ip);
            client->remote_ip = NULL;
        }    
        client->remote_ip = 0;
        client->remote_port = 0;
        client->recv_prot_len = 0;
        if(client->recvbuf)
        {
            msd_str_free(client->recvbuf);
            client->recvbuf = NULL;
        }
        if(client->sendbuf)
        {
            msd_str_free(client->sendbuf);
            client->sendbuf= NULL;
        }
        client->access_time = 0;

        client->worker_id = -1;
        return 0;
    }
    return MSD_OK;
}

/**
 * 功能: 任务分配
 * 参数: @client_idx: client的位置
 * 描述:
 *      1. 
 * 返回:成功:领命的woker的id，不返回; 失败:-x 
 **/
static int msd_thread_worker_dispatch(int client_idx)
{
    int res;
    int worker_id = msd_thread_list_find_next();
    msd_thread_pool_t *worker_pool = g_ins->pool;
    msd_thread_worker_t *worker;
    
    if (worker_id < 0)
    {
        return MSD_ERR;
    }

    worker = *(worker_pool->thread_worker_array+ worker_id);
    /* 任务分配写入通知! */
    res = write(worker->notify_write_fd, &client_idx, sizeof(int));
    if (res == -1)
    {
        MSD_ERROR_LOG("Pipe write error, errno:%d", errno);
        return MSD_ERR;
    }
    MSD_DEBUG_LOG("Notify the worker[%d] process the client[%d]", worker_id, client_idx);
    return worker_id;
}

/**
 * 功能: 在woker队列中找到一个能用的
 * 描述:
 *      1. 直接在woker队列中向后找，利用mod实现循环
 *      2. //TODO: 应该按照繁忙程度，找到最闲的woker，而不是轮训
 * 返回:成功:woker的索引. 失败:-x 
 **/
static int msd_thread_list_find_next()
{
    int i;
    int idx = -1;
    msd_master_t *master = g_ins->master;
    msd_thread_pool_t *pool = g_ins->pool;

    MSD_LOCK_LOCK(g_ins->thread_woker_list_lock);
    for (i = 0; i < pool->thread_worker_num; i++)
    {
        idx = (i + master->cur_thread + 1) % pool->thread_worker_num;
        if (*(pool->thread_worker_array + idx) && (*(pool->thread_worker_array + idx))->tid)
        {
            master->cur_thread = idx;
            break;
        }
    }
    MSD_LOCK_UNLOCK(g_ins->thread_woker_list_lock);

    if (i == pool->thread_worker_num)
    {
        MSD_ERROR_LOG("Thread pool full, Can not find a free worker!");
        idx = -1;
    }
    return idx;
}


/**
 * 功能: 关闭client
 * 参数: @client_idx
 *       @info，关闭的提示信息
 * 说明: 
 *    1. 
 **/
void msd_close_client(int client_idx, const char *info) 
{
    msd_conn_client_t **pclient;
    msd_conn_client_t  *client;
    msd_conn_client_t  *null = NULL;
    msd_thread_worker_t *worker = NULL;
    msd_dlist_node_t   *node;

    MSD_LOCK_LOCK(g_ins->client_conn_vec_lock);
    pclient = (msd_conn_client_t **)msd_vector_get_at(g_ins->master->client_vec,client_idx);
    client  = *pclient;

    /* 调用handle_close */
    if (g_ins->so_func->handle_close) 
    {
        g_ins->so_func->handle_close(client, info);
    }

    /* 删除client对应fd的ae事件和woker->client_list中成员 */
    worker = g_ins->pool->thread_worker_array[client->worker_id];
    if(worker)
    {
        msd_ae_delete_file_event(worker->t_ael, client->fd, MSD_AE_READABLE | MSD_AE_WRITABLE);
        if((node = msd_dlist_search_key(worker->client_list, client)))
        {
            MSD_INFO_LOG("Delete the client node[%d]. Addr:%s:%d", client->idx, client->remote_ip, client->remote_port);
            msd_dlist_delete_node(worker->client_list, node);
        }
        else
        {
            MSD_ERROR_LOG("Lost the client node[%d]. Addr:%s:%d", client->idx, client->remote_ip, client->remote_port);
        }
    }
    else
    {
        /* 任务下发失败，也会调用msd_close_client，此时worker_id等等
         * 相关信息还没和client进行关联，所以可能出现woker为空 */
        MSD_ERROR_LOG("The worker not found!!");
    }
    
    /* 删除conn_vec对应节点，msd_vctor_set_at的第三个参数是data的指针，
     * 而在处的data代表的client_conn的指针，所以第三个参数应该是个二级指针 
     **/
    msd_vector_set_at(g_ins->master->client_vec, client->idx, (void *)&null);
    g_ins->master->total_clients--;
    MSD_LOCK_UNLOCK(g_ins->client_conn_vec_lock);
    
    /* close掉client对应fd */
    close(client->fd);

    /* 调用msd_client_clear()清空client结构 */
    msd_client_clear(client);

    /* free client本身 */
    free(client);

    MSD_INFO_LOG("Close client[%d], info:%s", client->idx, info);

}

/**
 * 功能: master线程的时间回调函数。
 * 参数: @el
 *       @id，时间事件id
 *       @privdata，master指针
 * 说明: 
 *       定期统计全部client的信息，查看是否出现异常
 *       5分钟运行一次
 * 返回:成功:0; 失败:-x
 **/
static int msd_master_cron(msd_ae_event_loop *el, long long id, void *privdate) 
{
    msd_master_t *master = (msd_master_t *)privdate;     
    //msd_vector_iter_t iter = msd_vector_iter_new(master->client_vec);
    msd_conn_client_t **pclient;
    msd_conn_client_t *client;
    msd_thread_pool_t *pool = g_ins->pool;
    msd_thread_worker_t *worker;
    msd_dlist_iter    dlist_iter;
    msd_dlist_node_t  *node;
    int i;
    int master_client_cnt = 0;
    int worker_client_cnt = 0;
    
    MSD_INFO_LOG("Master cron begin!");
    /* 遍历client_vec */
    MSD_LOCK_LOCK(g_ins->client_conn_vec_lock);
    
    for( i=0; i < master->client_limit; i++)
    {
        pclient = (msd_conn_client_t **)msd_vector_get_at(master->client_vec, i);
        //printf("%p,             %p\n", pclient, *pclient);
        client  = *pclient;
        if( client && 0 != client->access_time )
        {
            master_client_cnt++;
        }
    }
    /*
    //段错误，之所以段错误，是因为用了msd_vector_iter_next，这个函数依赖vec->count，不靠谱
    do {
        pclient = (msd_conn_client_t **)(iter->data);
        client  = *pclient;
        printf("a\n");
        if(!(!client || 0 == client->access_time))
        {
            master_client_cnt++;
        }
    } while (msd_vector_iter_next(iter) == 0);
    */
    MSD_LOCK_UNLOCK(g_ins->client_conn_vec_lock);
    
    /* 遍历所有的worker中的client_list */
    MSD_LOCK_LOCK(g_ins->thread_woker_list_lock);
    for (i = 0; i < pool->thread_worker_num; i++)
    {
        if (*(pool->thread_worker_array + i) && (*(pool->thread_worker_array + i))->tid)
        {
            worker = *(pool->thread_worker_array+i);
            msd_dlist_rewind(worker->client_list, &dlist_iter);
            while ((node = msd_dlist_next(&dlist_iter))) 
            {
                client = node->value;
                if(client && 0 != client->access_time)
                {
                    worker_client_cnt++;
                }
            }
            
        }
    }
    MSD_LOCK_UNLOCK(g_ins->thread_woker_list_lock);    
    
    if(master->total_clients == master_client_cnt && master_client_cnt == worker_client_cnt )
    {
        MSD_INFO_LOG("Master cron end! Total_clients:%d, Master_client_cnt:%d, Worker_client_cnt:%d", 
            master->total_clients, master_client_cnt, worker_client_cnt);
    }
    else
    {
        //TODO 更加详细的任务信息!
        MSD_FATAL_LOG("Master cron end! Fatal Error!. Total_clients:%d, Master_client_cnt:%d, Worker_client_cnt:%d", 
            master->total_clients, master_client_cnt, worker_client_cnt);        
    }
    
    return (1000 * master->poll_interval);
}

/**
 * 功能: 主线程接收singal线程传过来的信号消息
 * 参数: @el, ae句柄 
 *       @fd, signal线程通知fd 
 *       @client_data, 额外参数
 *       @mask, 需要处理的事件 
 * 描述:
 *      1. 此函数是回调函数，所有的参数都是是由ae_main调用
 *         process_event函数的时候赋予的
 **/
static void msd_master_recv_signal(msd_ae_event_loop *el, int fd, 
        void *client_data, int mask)
{
    char msg[128];
    int read_len;
    
    MSD_AE_NOTUSED(el);
    MSD_AE_NOTUSED(mask);
    MSD_AE_NOTUSED(client_data);

    memset(msg, 0, 128);

    read_len = read(fd, msg, 4);
    if(read_len != 4)
    {
        MSD_ERROR_LOG("Master recv signal worker's notify error: %d", read_len);
        return;
    }
    
    MSD_DEBUG_LOG("Master recv signal worker's notify: %s", msg);
    if(strncmp(msg, "stop", 4) == 0)
    {
        //exit(0);
        msd_shut_down();
    }
    
    return;
}

/**
 * 功能: master发出mossad关闭指令
 * 描述:
 *      0. 关闭listen_fd，停止新的连接请求
 *      1. 向所有worker线程发出关闭指令，约定fd:-1
 *      2. 循环等待全部worker关闭连接(这步是最重要的，
 *         所有的工作都是为了它，让client能友好退出)
 *      3. 程序退出
 **/
static void msd_shut_down()
{
    msd_master_t *master    = g_ins->master;
    msd_thread_pool_t *pool = g_ins->pool;
    msd_thread_worker_t *worker;
    msd_conn_client_t  **pclient;
    msd_conn_client_t   *client;
    int master_client_cnt;
    int i, res, info;

    /* 关闭listen_fd，停止新的连接请求 */
    msd_ae_delete_file_event(master->m_ael, master->listen_fd, MSD_AE_READABLE);
    close(master->listen_fd); 
    /* 到此处，mossad处于半关闭，外界无法建立新连接了，但是原有连接仍然服务 */

    /* 向所有worker线程发出关闭指令，约定fd:-1 */
    MSD_LOCK_LOCK(g_ins->thread_woker_list_lock);
    for (i = 0; i < pool->thread_worker_num; i++)
    {
        if (*(pool->thread_worker_array + i) && (*(pool->thread_worker_array + i))->tid)
        {
            worker = *(pool->thread_worker_array+i);
            info = -1;
            res = write(worker->notify_write_fd, &info, sizeof(int));
            if (res == -1)
            {
                MSD_ERROR_LOG("Pipe write error, errno:%d", errno);
                MSD_LOCK_UNLOCK(g_ins->thread_woker_list_lock);  
                return;
            }
        }
    }
    MSD_LOCK_UNLOCK(g_ins->thread_woker_list_lock);  

    /* 循环等待所有的worker结束关闭 */
    do{
        /* 遍历client_vec，查看连接个数 */
        printf("bbbbbbbbbbbbbbbbbbbbbb\n");
        master_client_cnt = 0;
        MSD_LOCK_LOCK(g_ins->client_conn_vec_lock);
        for( i=0; i < master->client_limit; i++)
        {
            pclient = (msd_conn_client_t **)msd_vector_get_at(master->client_vec, i);
            client  = *pclient;
            if( client && 0 != client->access_time )
            {
                master_client_cnt++;
            }
        }
        MSD_LOCK_UNLOCK(g_ins->client_conn_vec_lock);

        if(master_client_cnt <= 0)
        {
            break;
        }
        else
        {
            printf("aaaaaaaaaaaaaaaaaa\n");
            msd_thread_usleep(10000);/* 10毫秒 */
        }
    }while(1);
    
    /* 销毁线程池 */
    msd_thread_pool_destroy(pool);

    return;
}



