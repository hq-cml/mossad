/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_thread.h 
 * 
 * Description :  Msd_thread, 工作线程相关实现.
 * 
 *     Created :  May 20, 2012 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/
#ifndef __MSD_THREAD_H_INCLUDED__
#define __MSD_THREAD_H_INCLUDED__

/*
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
       
#define MSD_PTHREAD_LOCK_MODE
//#define MSD_SYSVSEM_LOCK_MODE
//#define MSD_FCNTL_LOCK_MODE

#define MSD_LOG_MODE_THREAD

#include "msd_lock.h"
#include "msd_log.h"

#define MSD_OK   0
#define MSD_ERR -1
*/

#define THREAD_STACK_SIZE   10*1024*1024     /* 10M */

typedef struct thread_pool   msd_thread_pool_t;
typedef struct thread_worker msd_thread_worker_t;

typedef enum msd_worker_stat{
    W_RUNNING,
    W_STOPING,
    W_STOP
}msd_worker_stat_t;

struct thread_worker{
    int                idx;             /* 位于woker列表中的位置索引 */
    pthread_t          tid;             /* 线程id */
    int                notify_read_fd;  /* master和woker线程通信管道读取端 */
    int                notify_write_fd; /* master和woker线程通信管道写入端 */  
    msd_worker_stat_t  status;          /* worker的状态 */

    msd_ae_event_loop  *t_ael;          /* worker线程ae句柄，用于监听管道和所负责的client fd */
    msd_thread_pool_t  *pool;           /* 依附于的线程池句柄 */
    msd_dlist_t        *client_list;    /* 本线程负责的client，组成的队列 */
    
    //struct event_base *ev_base;
    //struct event ev_notify;
    //struct event ev_cleantimeout;

    //std::set<int> conn_set;
    //ares_channel dns_channel;
    //gkoAlloc mem;
    //void * userData;
};

struct thread_pool{
    msd_lock_t            *thread_lock;           /* 工作线程锁。各个平等的工作线程之间，需要同步的时候用此锁 */
    msd_thread_worker_t   **thread_worker_array;  /* worker线程列表 */
    
    int                   thread_worker_num;      /* thread_worker_array数组长度 */
    int                   thread_stack_size;      /* 线程栈大小 */
    
    int                   client_timeout;         /* client超时时间 */
    int                   poll_interval;          /* worker线程Cron频率 */
    //pthread_cond_t      cond;
    //pthread_cond_t      exit_cond;
    //dqueue_t            task_queue;
    //int                 exit;
    //int                 threads_idle;
    //
    //int                 threads_max;
};
 
msd_thread_pool_t *msd_thread_pool_create(int worker_num, int stack_size , void* (*worker_task)(void *arg));
int msd_thread_pool_destroy(msd_thread_pool_t *pool);
void* msd_thread_worker_cycle(void* arg);
void msd_thread_sleep(int s);
void msd_thread_usleep(int s);

#endif /* __MSD_THREAD_H_INCLUDED__ */

