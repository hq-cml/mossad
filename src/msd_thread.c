/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_thread.c
 * 
 * Description :  Msd_thread, 工作线程相关实现.
 * 
 *     Created :  Apr 20, 2012 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
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

static int msd_thread_worker_destroy(msd_thread_worker_t *worker);

static void msd_read_from_client(msd_ae_event_loop *el, int fd, void *privdata, int mask);

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
        (msd_thread_worker_t **)calloc(worker_num, sizeof(msd_thread_worker_t))))
    {
        MSD_LOCK_DESTROY(pool->thread_lock);
        free(pool);
        return NULL;
    }
    
    pool->thread_worker_num = worker_num;
    pool->thread_stack_size = stack_size;    
    
    /* 初始化启动woker线程 */
    for (i = 0; i < worker_num; ++i) 
    {
        if(msd_thread_worker_create(pool, worker_task, i) != MSD_OK)
        {
            MSD_ERROR_LOG("Msd_thread_worker_create failed, idx:%d", i);
            return NULL;
        }
    }
    
    /* 初始化启动信号处理线程 */
    //TODO
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
    MSD_LOCK_LOCK(pool->thread_lock);
    for(i=0; i<pool->thread_worker_num; i++)
    {
        msd_thread_worker_destroy(pool->thread_worker_array[i]);
    }
    MSD_LOCK_UNLOCK(pool->thread_lock);
    MSD_LOCK_DESTROY(pool->thread_lock);
    free(pool->thread_worker_array);
    free(pool);
    return MSD_OK;
}

/**
 * 功能: 销毁worker线程池
 * 参数: @pool: 线程池句柄 
 * 返回:成功:0; 失败:-x
 **/
static int msd_thread_worker_destroy(msd_thread_worker_t *worker) 
{
    assert(worker);
    //TODO
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
 * 功能: worker线程工作函数
 * 参数: @arg: 线程函数，通常来说，都是线程自身句柄的指针
 * 说明: 
 *       1.  
 * 返回:成功:0; 失败:-x
 **/
void* msd_thread_worker_cycle(void* arg) 
{
    msd_thread_worker_t *worker = (msd_thread_worker_t *)arg;
    MSD_INFO_LOG("Thread no.%d begin to work", worker->idx);

    
    if (msd_ae_create_file_event(worker->t_ael, worker->notify_read_fd, 
                MSD_AE_READABLE, msd_thread_worker_process_notify, arg) == MSD_ERR) 
    {
        MSD_ERROR_LOG("Create file event failed");
        msd_ae_free_event_loop(worker->t_ael);
        return (void *)NULL; 

    }

    //TODO
    //启动定时器，清理超时的client

    
    msd_ae_main_loop(worker->t_ael);
 
    return (void*)NULL;
}

/**
 * 功能: worker线程工作函数
 * 参数: @arg: 线程函数，通常来说，都是线程自身句柄的指针
 * 说明: 
 *       1.  
 * 返回:成功:0; 失败:-x
 **/
static void msd_thread_worker_process_notify(struct msd_ae_event_loop *el, int notify_fd, 
        void *client_data, int mask)
{
    msd_thread_worker_t *worker = (msd_thread_worker_t *)client_data;
    msd_master_t        *master = g_ins->master;
    int                  client_idx;
    msd_conn_client_t   **pclient = NULL;
    int                  read_len;

    /* 领取client idx */
    if((read_len = read(notify_fd, &client_idx, sizeof(int)))!= sizeof(int))
    {
        MSD_ERROR_LOG("Worker[%d] read the notify_fd error! read_len:%d", worker->idx, read_len);
        return;
    }
    MSD_INFO_LOG("Worker[%d] Get the task. Client_idx[%d]", worker->idx, client_idx);

    /* 根据client idx获取到client地址 */
    pclient = (msd_conn_client_t **)msd_vector_get_at(master->client_vec, client_idx);

    /* 将worker_id写入client结构 */
    (*pclient)->worker_id = worker->idx;
    //TODO 应该加一个向woker->conn自增之类的操作

    
    /* 欢迎信息 */
    if(g_ins->so_func->handle_open)
    {
        if(MSD_OK != g_ins->so_func->handle_open(*pclient))
        {
            msd_close_client(client_idx, NULL);
            MSD_ERROR_LOG("Handle open error! Close connection. IP:%s, Port:%d.", 
                            (*pclient)->remote_ip, (*pclient)->remote_port);
            return;
        }
    }
        
    /* 注册读取事件 */
    if (msd_ae_create_file_event(worker->t_ael, (*pclient)->fd, MSD_AE_READABLE,
                msd_read_from_client, *pclient) == MSD_ERR) 
    {
        msd_close_client(client_idx, "create file event failed");
        MSD_ERROR_LOG("Create read file event failed for connection:%s:%d",
                (*pclient)->remote_ip, (*pclient)->remote_port);
        return;
    }  
}

/**
 * 功能: client可读时的回调函数。
 * 参数: @arg: 线程函数，通常来说，都是线程自身句柄的指针
 * 说明: 
 *       1.  
 * 返回:成功:0; 失败:-x
 **/
static void msd_read_from_client(msd_ae_event_loop *el, int fd, void *privdata, int mask) 
{
    msd_conn_client_t *client = (msd_conn_client_t *)privdata;
    int nread;
    char buf[MSD_IOBUF_SIZE];
    MSD_AE_NOTUSED(el);  /* 不使用，啥也不干 */
    MSD_AE_NOTUSED(mask);
    
    MSD_DEBUG_LOG("Read from client %s:%d", client->remote_ip, client->remote_port);
    
    nread = read(fd, buf, MSD_IOBUF_SIZE - 1);
    client->access_time = time(NULL);
    if (nread == -1) 
    {
        /* 非阻塞的fd，读取不会阻塞，如果无内容可读，则errno==EAGAIN */
        if (errno == EAGAIN) 
        {
            MSD_WARNING_LOG("Read connection %s:%d eagain: %s",
                     client->remote_ip, client->remote_port, strerror(errno));
            nread = 0;
            return;
        } 
        else 
        {
            MSD_ERROR_LOG("Read connection %s:%d failed: %s",
                     client->remote_ip, client->remote_port, strerror(errno));

            msd_close_client(client->idx, "Read data failed!");
            return;
        }
    } 
    else if (nread == 0) 
    {
        MSD_INFO_LOG("Client close connection %s:%d",client->remote_ip, client->remote_port);
        msd_close_client(client->idx, NULL);
        return;
    }
    buf[nread] = '\0';

    //printf("%d %d  %d  %d\n", buf[nread-3], buf[nread-2], buf[nread-1], buf[nread]);
    MSD_INFO_LOG("Read from client %s:%d. Length:%d, Content:%s", 
                    client->remote_ip, client->remote_port, nread, buf);

    /* 将读出的内容拼接到recvbuf上面，这里是拼接，因为可能请求的包很大
     * 需要多次read才能将读取内容拼装成一个完整的请求 */
    msd_str_cat(&(client->recvbuf), buf);

    if (client->recv_prot_len == 0) 
    {
        /* 每次读取的长度不定，所以recv_prot_len是一个变量，不断调整 */ 
        client->recv_prot_len = g_ins->so_func->handle_input(client);
        MSD_DEBUG_LOG("Get the recv_prot_len %d", client->recv_prot_len);
    } 

    if (client->recv_prot_len < 0 || client->recv_prot_len > MSD_MAX_PROT_LEN) 
    {
        MSD_ERROR_LOG("Invalid protocol length:%d for connection %s:%d", 
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
        MSD_INFO_LOG("Unkonw the accurate protocal lenght, do noting!. connection %s:%d", 
                        client->remote_ip, client->remote_port);
        return;
    } 

    if ( client->recvbuf->len >= client->recv_prot_len) 
    {
        //TODO 这个地方还需要多一些例子测测，比如http，transfer等，
        //各个分支都走一遍，光用一个echo不够
        msd_close_client(client->idx, "fuck!");return;
        /* 目前读取到的数据，已经足够拼出一个完整请求包，则调用handle_process */
        if(MSD_OK != g_ins->so_func->handle_process(client))
        {
            MSD_ERROR_LOG("The handle_process failed. connection %s:%d", 
                        client->remote_ip, client->remote_port);
        }
        else
        {
            MSD_DEBUG_LOG("The handle_process success. connection %s:%d", 
                        client->remote_ip, client->remote_port);
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
                              
    }
    else
    {
        /* 目前读取到的数据还不够组装成需要的请求，则继续等待读取 */
        MSD_DEBUG_LOG("The data lenght not enough, do noting!. connection %s:%d", 
                        client->remote_ip, client->remote_port);
    }
    return;
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
