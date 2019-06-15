/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_thread.c
 * 
 * Description :  Msd_thread, 工作线程相关实现.
 * 
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/
#include "msd_core.h"

#define MSD_IOBUF_SIZE    4096
#define MSD_MAX_PROT_LEN  10485760 /* 最大请求包10M */


extern msd_instance_t *g_ins;

static void msd_thread_worker_process_notify(struct msd_ae_event_loop *el, int fd, 
        void *client_data, int mask);
static int msd_thread_worker_create(msd_thread_pool_t *pool, void* (*worker_task)(void *arg), 
        int idx);
static void msd_read_from_client(msd_ae_event_loop *el, int fd, void *privdata, int mask);
static int msd_thread_worker_cron(msd_ae_event_loop *el, long long id, void *privdate);
static void msd_thread_worker_shut_down(msd_thread_worker_t *worker);

/**
 * 功能:创建线程池
 * 参数:@num: 工作线程个数 
 *      @statc_size: 工作线程栈大小 
 * 描述:
 *      1. 
 * 返回:成功:线程池地址;失败:NULL
 **/
msd_thread_pool_t *msd_thread_pool_create(int worker_num, int stack_size , void* (*worker_task)(void*)) 
{
    msd_thread_pool_t *pool;
    int i;
    assert(worker_num > 0 && stack_size >= 0);

    /* Allocate memory and zero all them. */
    pool = (msd_thread_pool_t *)calloc(1, sizeof(*pool));
    if (!pool) {
        return NULL;
    }

    /* 初始化线程锁 */
    if (MSD_LOCK_INIT(pool->thread_lock) != 0) 
    {
        free(pool);
        return NULL;
    }

    /* 初始化线程worker列表 */
    if(!(pool->thread_worker_array = 
        (msd_thread_worker_t **)calloc(worker_num, sizeof(msd_thread_worker_t *))))
    {
        MSD_LOCK_DESTROY(pool->thread_lock);
        free(pool);
        return NULL;
    }
    
    pool->thread_worker_num = worker_num;
    pool->thread_stack_size = stack_size;    

    /* 初始化client超时时间，默认5分钟 */
    pool->client_timeout =  msd_conf_get_int_value(g_ins->conf, "client_timeout", 300);

    /* 初始化cron频率 */
    pool->poll_interval  =  msd_conf_get_int_value(g_ins->conf, "worker_poll_interval", 60);
    
    /* 初始化启动woker线程 */
    for (i = 0; i < worker_num; ++i) 
    {
        if(msd_thread_worker_create(pool, worker_task, i) != MSD_OK)
        {
            MSD_ERROR_LOG("Msd_thread_worker_create failed, idx:%d", i);
            return NULL;
        }
    }

    return pool;
}

/**
 * 功能:创建工作线程
 * 参数:@pool: 线程池句柄 
 *      @worker: 工作函数
 *      @idx: 线程位置索引号 
 * 描述:
 *      1. 首先创建woker线程的资源，并初始化
 *      2. 将线程句柄放入主线程的容器中的对应位置
 *      3. 启动woker线程，开始干活，函数返回
 * 返回:成功:0; 失败:-x
 **/
static int msd_thread_worker_create(msd_thread_pool_t *pool, void* (*worker_task)(void *), int idx)
{
    msd_thread_worker_t *worker  = NULL;
    char error_buf[256];
    if(!(worker = calloc(1, sizeof(msd_thread_worker_t))))
    {
        MSD_ERROR_LOG("Thread_worker_creat failed, idx:%d", idx);
        return MSD_ERR;
    }
    
    worker->pool                 = pool;
    worker->idx                  = idx;
    worker->status               = W_STOP;
    worker->priv_data            = NULL;
    pool->thread_worker_array[idx] = worker; /* 放入对应的位置 */

    /* 
     * 创建通信管道
     * master线程将需要处理的fd写入管道，woker线程从管道取出
     **/
    int fds[2];
    if (pipe(fds) != 0)
    {
        free(worker);
        MSD_ERROR_LOG("Thread_worker_creat pipe failed, idx:%d", idx);
        return MSD_ERR;
    }
    worker->notify_read_fd  = fds[0];
    worker->notify_write_fd = fds[1];

    /* 管道不能是阻塞的! */
    if((MSD_OK != msd_anet_nonblock(error_buf,  worker->notify_read_fd))
        || (MSD_OK != msd_anet_nonblock(error_buf,  worker->notify_write_fd)))
    {
        close(worker->notify_read_fd);
        close(worker->notify_write_fd);
        free(worker);
        MSD_ERROR_LOG("Set pipe nonblock failed, err:%s", error_buf);
        return MSD_ERR;        
    }

    /* 创建ae句柄 */
    if(!(worker->t_ael = msd_ae_create_event_loop()))
    {
        close(worker->notify_read_fd);
        close(worker->notify_write_fd);
        free(worker);
        MSD_ERROR_LOG("Create ae failed");
        return MSD_ERR;  
    }

    /* 初始化client队列 */
    if(!(worker->client_list = msd_dlist_init()))
    {
        close(worker->notify_read_fd);
        close(worker->notify_write_fd);
        msd_ae_free_event_loop(worker->t_ael);
        free(worker);
        MSD_ERROR_LOG("Create ae failed");
        return MSD_ERR;         
    }

    /* 创建线程 */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, pool->thread_stack_size);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if(pthread_create(&worker->tid, &attr, worker_task, worker) != 0)
    {
        close(worker->notify_read_fd);
        close(worker->notify_write_fd);
        free(worker);
        MSD_ERROR_LOG("Thread_worker_creat failed, idx:%d", idx);
        return MSD_ERR;        
    }
    pthread_attr_destroy(&attr);
    return MSD_OK;    
}

/**
 * 功能: 销毁线程池
 * 参数: @pool: 线程池句柄 
 * 返回:成功:0; 失败:-x
 **/
int msd_thread_pool_destroy(msd_thread_pool_t *pool) 
{
    assert(pool);
    int i;
    msd_thread_worker_t *worker;
    MSD_LOCK_LOCK(g_ins->thread_woker_list_lock);
    for(i=0; i<pool->thread_worker_num; i++)
    {
        worker = pool->thread_worker_array[i];
        if(worker)
        {
            free(worker);
        }
    }
    MSD_LOCK_UNLOCK(g_ins->thread_woker_list_lock);
    MSD_LOCK_DESTROY(pool->thread_lock);
    free(pool->thread_worker_array);
    free(pool);
    MSD_INFO_LOG("Destory the thread pool!");
    return MSD_OK;
}

/**
 * 功能: 线程睡眠函数
 * 参数: @sec, 睡眠时长，秒数
 * 说明: 
 *       1.利用select变相实现睡眠效果
 **/
void msd_thread_sleep(int s) 
{
    struct timeval timeout;
    timeout.tv_sec  = s;
    timeout.tv_usec = 0;
    select( 0, NULL, NULL, NULL, &timeout);
}

/**
 * 功能: 线程睡眠函数
 * 参数: @sec, 睡眠时长，秒数
 * 说明: 
 *       1.利用select变相实现睡眠效果
 **/
void msd_thread_usleep(int s) 
{
    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = s;
    select( 0, NULL, NULL, NULL, &timeout);
}


/**
 * 功能: worker线程工作函数
 * 参数: @arg: 线程函数，通常来说，都是线程自身句柄的指针
 * 说明: 
 *       1.msd_ae_free_event_loop不应该在ae的回调里面调用，在回调里面应该调用
 *         msd_ae_stop，导致msd_ae_main_loop退出，然后ae的析构放在main_loop后
 * 返回:成功:0; 失败:-x
 **/
void* msd_thread_worker_cycle(void* arg) 
{
    msd_thread_worker_t *worker = (msd_thread_worker_t *)arg;
    MSD_INFO_LOG("Worker[%d] begin to work", worker->idx);

    worker->status = W_RUNNING;

    /* worker线程初始化函数 */
    if (g_ins->so_func->handle_worker_init) 
    {
        if (g_ins->so_func->handle_worker_init(g_ins->conf, worker) != MSD_OK) 
        {
            MSD_ERROR_LOG("Invoke hook handle_worker_init failed");
            MSD_BOOT_FAILED("Invoke hook handle_worker_init failed");
        }
    }

    
    if (msd_ae_create_file_event(worker->t_ael, worker->notify_read_fd, 
                MSD_AE_READABLE, msd_thread_worker_process_notify, arg) == MSD_ERR) 
    {
        MSD_ERROR_LOG("Create file event failed");
        msd_ae_free_event_loop(worker->t_ael);
        return (void *)NULL; 

    }

    /* 启动定时器，清理超时的client */
    msd_ae_create_time_event(worker->t_ael, 1000*worker->pool->poll_interval,
            msd_thread_worker_cron, worker, NULL);

    /* 无限循环 */
    msd_ae_main_loop(worker->t_ael);

    /* 退出了ae_main_loop，线程的生命走到尽头 */
    worker->tid = 0;
    worker->idx = -1;
    msd_dlist_destroy(worker->client_list);
    msd_ae_free_event_loop(worker->t_ael);
    //free(worker);销毁池子的时候会统一释放
    pthread_exit(0);
    return (void*)NULL;
}

/**
 * 功能: worker线程工作函数
 * 参数: @arg: 线程函数，通常来说，都是线程自身句柄的指针
 * 说明: 
 *       1. 如果读出的client_idx是-1，则表示需要程序即将关闭
 *       2. 如果读出的client_idx非-1，则根据idx取出client句柄
 *          将client->fd加入读取事件集合，进行管理
 * 返回:成功:0; 失败:-x
 **/
static void msd_thread_worker_process_notify(struct msd_ae_event_loop *el, int notify_fd, 
        void *client_data, int mask)
{
    msd_thread_worker_t *worker = (msd_thread_worker_t *)client_data;
    msd_master_t        *master = g_ins->master;
    int                  client_idx;
    msd_conn_client_t   **pclient = NULL;
    msd_conn_client_t    *client = NULL;
    int                  read_len;

    /* 领取client idx */
    if((read_len = read(notify_fd, &client_idx, sizeof(int)))!= sizeof(int))
    {
        MSD_ERROR_LOG("Worker[%d] read the notify_fd error! read_len:%d", worker->idx, read_len);
        return;
    }
    MSD_INFO_LOG("Worker[%d] Get the task. Client_idx[%d]", worker->idx, client_idx);

    /* 如果读出的client_idx是-1，则表示需要程序即将关闭 */
    if(-1 == client_idx)
    {
        msd_thread_worker_shut_down(worker);
        return;
    }
    
    /* 根据client idx获取到client地址 */
    MSD_LOCK_LOCK(g_ins->client_conn_vec_lock);
    pclient = (msd_conn_client_t **)msd_vector_get_at(master->client_vec, client_idx);
    MSD_LOCK_UNLOCK(g_ins->client_conn_vec_lock);

    if(pclient && *pclient)
    {
        client = *pclient;
    }
    else
    {
        MSD_ERROR_LOG("Worker[%d] get the client error! client_idx:%d", worker->idx, client_idx);
        return;
    }

    /* 将worker_id写入client结构 */
    client->worker_id = worker->idx;
    client->status    = C_WAITING;
    
    /* 向worker->client_list追加client */
    msd_dlist_add_node_tail(worker->client_list, client);
    
    /* 欢迎信息 */
    if(g_ins->so_func->handle_open)
    {
        if(MSD_OK != g_ins->so_func->handle_open(client))
        {
            msd_close_client(client_idx, NULL);
            MSD_ERROR_LOG("Handle open error! Close connection. IP:%s, Port:%d.", 
                            client->remote_ip, client->remote_port);
            return;
        }
    }
        
    /* 注册读取事件 */
    if (msd_ae_create_file_event(worker->t_ael, client->fd, MSD_AE_READABLE,
                msd_read_from_client, client) == MSD_ERR) 
    {
        msd_close_client(client_idx, "create file event failed");
        MSD_ERROR_LOG("Create read file event failed for connection:%s:%d",
                client->remote_ip, client->remote_port);
        return;
    }  
}

/**
 * 功能: client可读时的回调函数。
 * 参数: @privdata，回调函数的入参
 * 说明: 
 *       1.  
 * 返回:成功:0; 失败:-x
 **/
static void msd_read_from_client(msd_ae_event_loop *el, int fd, void *privdata, int mask) 
{
    msd_conn_client_t *client = (msd_conn_client_t *)privdata;
    int nread;
    char buf[MSD_IOBUF_SIZE];
    //MSD_AE_NOTUSED(el);
    MSD_AE_NOTUSED(mask);
    int ret;
    msd_thread_worker_t *worker;
    
    MSD_DEBUG_LOG("Read from client %s:%d", client->remote_ip, client->remote_port);
    
    nread = read(fd, buf, MSD_IOBUF_SIZE - 1);
    client->access_time = time(NULL);
    if (nread == -1) 
    {
        /* 非阻塞的fd，读取不会阻塞，如果无内容可读，则errno==EAGAIN */
        if (errno == EAGAIN) 
        {
            MSD_WARNING_LOG("Read connection [%s:%d] eagain: %s",
                     client->remote_ip, client->remote_port, strerror(errno));
            nread = 0;
            return;
        }
        else if (errno == EINTR) 
        {
            MSD_WARNING_LOG("Read connection [%s:%d] interrupt: %s",
                     client->remote_ip, client->remote_port, strerror(errno));
            nread = 0;
            return;
        } 
        else 
        {
            MSD_ERROR_LOG("Read connection [%s:%d] failed: %s",
                     client->remote_ip, client->remote_port, strerror(errno));

            msd_close_client(client->idx, "Read data failed!");
            return;
        }
    } 
    else if (nread == 0) 
    {
        MSD_INFO_LOG("Client close connection. IP:%s, Port:%d",client->remote_ip, client->remote_port);
        msd_close_client(client->idx, NULL);
        return;
    }
    buf[nread] = '\0';

    //printf("%d %d  %d  %d\n", buf[nread-3], buf[nread-2], buf[nread-1], buf[nread]);
    MSD_INFO_LOG("Read from client. IP:%s, Port:%d. Length:%d, Content:%s", 
                    client->remote_ip, client->remote_port, nread, buf);

    /* 将读出的内容拼接到recvbuf上面，这里是拼接，因为可能请求的包很大
     * 需要多次read才能将读取内容拼装成一个完整的请求 */
    msd_str_cat(&(client->recvbuf), buf);

    /* 一次接受的内容可能是多个完整的协议包 */
    do{
        if (client->recv_prot_len == 0) 
        {
            /* 每次读取的长度不定，所以recv_prot_len是一个变量，不断调整 */ 
            client->recv_prot_len = g_ins->so_func->handle_prot_len(client);
            MSD_DEBUG_LOG("Get the recv_prot_len %d", client->recv_prot_len);
        } 

        if (client->recv_prot_len < 0 || client->recv_prot_len > MSD_MAX_PROT_LEN) 
        {
            MSD_ERROR_LOG("Invalid protocol length:%d for connection. IP:%s, Port:%d", 
                     client->recv_prot_len, client->remote_ip, client->remote_port);
            msd_close_client(client->idx, "Wrong recv_port_len!");
            return;
        } 

        if (client->recv_prot_len == 0) 
        {
            /* 当处理一个比较大型的请求包的时候(或者客户端是telnet，每一次回车都会触发一个TCP包发送)
             * 一个请求可能会由多个TCP包发送过来andle_input暂时还无法判断出整个请求的长度，则返回0，
             * 表示需要继续读取数据，直到handle_input能够判断出请求长度为止
             */
            MSD_INFO_LOG("Unkonw the accurate protocal length, do noting!. Connection. IP:%s, Port:%d", 
                            client->remote_ip, client->remote_port);
            client->status = C_RECEIVING;
            return;
        } 

        if ( client->recvbuf->len >= client->recv_prot_len) 
        {
            client->status = C_PROCESSING;
            /* 目前读取到的数据，已经足够拼出一个完整请求包，则调用handle_process */
            ret = g_ins->so_func->handle_process(client);
            
            if(MSD_OK == ret)
            {
                /* 返回O，表示成功并继续 */
                MSD_INFO_LOG("The handle_process success. Continue. Connection. IP:%s, Port:%d", 
                            client->remote_ip, client->remote_port);
            }
            else if(MSD_END == ret)
            {
                /* 返回MSD_END，表示成功但不继续 */
                MSD_INFO_LOG("The handle_process success. End. Connection. IP:%s, Port:%d", 
                            client->remote_ip, client->remote_port);
                //msd_close_client(client->idx, NULL);
                //return;
                client->close_conn = 1; /* response成功之后，自动关闭连接 */
                msd_ae_delete_file_event(el, fd, MSD_AE_READABLE);
            }
            else
            {
                /* 处理失败，直接关闭client */
                MSD_ERROR_LOG("The handle_process failed. End. Connection. IP:%s, Port:%d", 
                            client->remote_ip, client->remote_port);
                msd_close_client(client->idx, NULL);
                return;
            }

            /* 每次只读取recv_prot_len数据长度，如果recvbuf里面还有剩余数据，则应该截出来保留 */
            if(MSD_OK != msd_str_range(client->recvbuf, client->recv_prot_len,  client->recvbuf->len - 1))
            {
                /* 当recv_prot_len==recvbuf->len的时候，range()返回错误，直接清空缓冲区 */
                msd_str_clear(client->recvbuf);
            }
                
            /* 将协议长度清0，因为每次请求都是独立的，请求的协议长度是可能发生变化，比如http服务器 
               * 需要由handle_input函数去实时计算 */
            client->recv_prot_len = 0; 
            client->status        = C_WAITING;

            /* 如果sendbuf不为空，则注册Client写回事件 */
            if(client->sendbuf->len > 0)
            {
                worker = msd_get_worker(client->worker_id);
                if (msd_ae_create_file_event(worker->t_ael, client->fd, MSD_AE_WRITABLE,
                            msd_write_to_client, client) == MSD_ERR) 
                {
                    msd_close_client(client->idx, "create file event failed");
                    MSD_ERROR_LOG("Create write file event failed. Connection. IP:%s, Port:%d", client->remote_ip, client->remote_port);
                    return;
                }
            }
            
            /* 杜绝空转 */
            if(client->recvbuf->len <= 0)
            {
                return;
            }                     
        }
        else
        {
            /* 目前读取到的数据还不够组装成需要的请求，则继续等待读取 */
            MSD_DEBUG_LOG("The data lenght not enough, do noting!. IP:%s, Port:%d", 
                            client->remote_ip, client->remote_port);
            return;
        }
    }while(1);
    return;
}

/**
 * 功能: 向client回写response
 * 参数: @privdata，回调函数的入参
 * 说明: 
 *       1.  
 * 返回:成功:0; 失败:-x
 **/
void msd_write_to_client(msd_ae_event_loop *el, int fd, void *privdata, int mask) 
{
    msd_conn_client_t *client = (msd_conn_client_t *)privdata;
    int nwrite;
    MSD_AE_NOTUSED(mask);
    
    MSD_DEBUG_LOG("Write to client. IP:%s, Port:%d", client->remote_ip, client->remote_port);

    nwrite = write(fd, client->sendbuf->buf, client->sendbuf->len);
    client->access_time = time(NULL);
    if (nwrite < 0) 
    {
        if (errno == EAGAIN) /* 非阻塞fd写入，可能暂不可用 */
        {
            MSD_WARNING_LOG("Write to client temporarily unavailable! IP:%s, Port:%d.", client->remote_ip, client->remote_port);
            nwrite = 0;
        } 
        else if(errno==EINTR)/* 遭遇中断 */
        {  
            MSD_WARNING_LOG("Handle process was interupted! IP:%s, Port:%d. Error:%s.", client->remote_ip, client->remote_port, strerror(errno));
            nwrite = 0; 
        } 
        else 
        {
            MSD_ERROR_LOG("Write to client %s:%d failed:%s", client->remote_ip, client->remote_port, strerror(errno));
            msd_close_client(client->idx, NULL);
            return;
        }
    }
    MSD_INFO_LOG("Write to client! IP:%s, Port:%d. Write_len:%d", client->remote_ip, client->remote_port, nwrite);

    /* 将已经写完的数据，从sendbuf中裁剪掉 */
    if(MSD_OK != msd_str_range(client->sendbuf, nwrite, client->sendbuf->len-1))
    {
        /* 如果已经写完了,则write_len == client->sendbuf->len。则msd_str_range返回NONEED */
        msd_str_clear(client->sendbuf);
    }

     /* 水平触发，如果sendbuf已经为空，则删除写事件，否则会不断触发
       * 注册写事件有so中的handle_process去完成 */
    if(client->sendbuf->len == 0)
    {
        msd_ae_delete_file_event(el, fd, MSD_AE_WRITABLE);

        if (client->close_conn) 
        {
            msd_close_client(client->idx, NULL);
        }
    }
    return;
}

/**
 * 功能: worker线程的时间回调函数。
 * 参数: @el
 *       @id，时间事件id
 *       @privdata，worker指针
 * 说明: 
 *       遍历clients数组，找出满足超时条件的client节点
 *       关闭之,每分钟运行一次 
 * 返回:成功:0; 失败:-x
 **/
static int msd_thread_worker_cron(msd_ae_event_loop *el, long long id, void *privdate) 
{
    msd_thread_worker_t *worker = (msd_thread_worker_t *)privdate;
    msd_dlist_iter_t    iter;
    msd_dlist_node_t    *node;
    msd_conn_client_t   *cli;
    time_t              unix_clock = time(NULL);

    MSD_INFO_LOG("Worker[%d] cron begin! Client count:%d", worker->idx, worker->client_list->len);
     
    msd_dlist_rewind(worker->client_list, &iter);

    while ((node = msd_dlist_next(&iter))) 
    {
        cli = node->value;
        /* client超时判断，默认是60秒超时 */
        if ( unix_clock - cli->access_time > worker->pool->client_timeout) 
        {
            MSD_DEBUG_LOG("Connection %s:%d timeout closed", 
                    cli->remote_ip, cli->remote_port);
            msd_close_client(cli->idx, "Time out!");
        }
    }
    MSD_INFO_LOG("Worker[%d] cron end! Client count:%d", worker->idx, worker->client_list->len);
    return (1000 * worker->pool->poll_interval);
}

/**
 * 功能: 关闭自己负责的全部client
 * 参数: @el
 *       @id，时间事件id
 *       @privdata，worker指针
 * 说明: 
 *       1. 遍历clients数组，关闭全部client 
 *       2. 销毁所有成员的资源
 *       3. 线程退出
 * 返回:成功:0; 失败:-x
 **/
static void msd_thread_worker_shut_down(msd_thread_worker_t *worker)
{
    msd_dlist_iter_t    iter;
    msd_dlist_node_t    *node;
    msd_conn_client_t   *cli;

    MSD_INFO_LOG("Worker[%d] begin to shutdown! Client count:%d", worker->idx, worker->client_list->len);
    worker->status = W_STOPING;
    /* 遍历 */
    msd_dlist_rewind(worker->client_list, &iter);
    while ((node = msd_dlist_next(&iter))) 
    {
        cli = node->value;
        msd_close_client(cli->idx, "Mossad is shutting down!");
    }
    MSD_INFO_LOG("Worker[%d] end shutdown! Client count:%d", worker->idx, worker->client_list->len);    

    /* 去除事件，并将AE停止 */
    msd_ae_delete_file_event(worker->t_ael, worker->notify_read_fd, MSD_AE_READABLE | MSD_AE_WRITABLE);
    msd_ae_stop(worker->t_ael);
    close(worker->notify_read_fd);
    close(worker->notify_write_fd);    
}

/**
 * 功能: 根据worker的id，获得worker句柄
 * 返回: 成功:worker句柄; 失败:-x
 **/
msd_thread_worker_t * msd_get_worker(int worker_id)
{
    msd_thread_worker_t *worker;

    if(worker_id < 0 || worker_id > g_ins->pool->thread_worker_num-1)
    {
        return NULL;
    }
    
    worker = *(g_ins->pool->thread_worker_array+ worker_id);  
    return worker;
}


#ifdef __MSD_THREAD_TEST_MAIN__

int g_int = 0;
int fd;

//测试线程锁
//如果不加锁，则最终的文件可能行数不够，或者顺序不对
//如果加锁，则最终的文件可能行数或者顺序都完全正确
void* task(void* arg) 
{
    msd_thread_worker_t *worker = (msd_thread_worker_t *)arg;
    
    MSD_LOCK_LOCK(worker->pool->thread_lock);
    msd_thread_sleep(worker->idx); // 睡眠
    g_int++;
    printf("I am thread %d, I get the g_int:%d. My tid:%lu, pool:%lu\n", worker->idx, g_int, (long unsigned int)worker->tid, (long unsigned int)worker->pool);
    write(fd, "hello\n", 6);
    write(fd, "abcde\n", 6);
    MSD_LOCK_UNLOCK(worker->pool->thread_lock);
    
    return NULL;
}

int main(int argc, char *argv[]) 
{
    int i=0;
    fd = open("test.txt", O_WRONLY|O_CREAT, 0777);
    msd_log_init("./","thread.log", 4, 10*1024*1024, 1, 0); 
    //msd_thread_pool_create(1000, 1024*1024, task);
    msd_thread_pool_t *pool = msd_thread_pool_create(10, 1024*1024, task);
    msd_thread_sleep(1);//睡眠一秒，让线程充分启动起来！
    
    /* 注意，如果线程是detache状态，下面的join将失效
     * 但是，detached的线程，不会因为主线程退出而退出！！！
     */
    for(i=0; i<10; i++)
    {
        pthread_join(pool->thread_worker_array[i]->tid, NULL);
        printf("Thread %d(%lu) exit\n", i, pool->thread_worker_array[i]->tid);
    }
    printf("The final val:%d\n", g_int);
    msd_thread_pool_destroy(pool);
    return 0;
}

#endif /* __MSD_THREAD_TEST_MAIN__ */

