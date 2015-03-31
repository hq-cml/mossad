/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *     Filename:  Msd_core.h 
 * 
 *  Description:  The massod's public header file. 
 * 
 *      Version:  1.0.0
 * 
 *       Author:  HQ 
 *
 **/

#ifndef __MSD_CORE_H_INCLUDED__ 
#define __MSD_CORE_H_INCLUDED__

/* --------SYSTEM HEADER FILE---------- */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <strings.h>
#include <libgen.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <getopt.h>
#include <dlfcn.h>
#include <sys/prctl.h>
#include <signal.h>
#include <sys/resource.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <sys/cdefs.h>
#include <execinfo.h>

 
/* -------------------CONFIG------------------- */
/* Lock mode */
#ifdef __MSD_PTHREAD_LOCK_MODE__
    #define MSD_PTHREAD_LOCK_MODE        //线程Mutex
#elif defined __MSD_SYSVSEM_LOCK_MODE__
    #define MSD_SYSVSEM_LOCK_MODE        //SYSVSEM
#elif defined __MSD_FCNTL_LOCK_MODE__
    #define MSD_FCNTL_LOCK_MODE          //文件锁
#endif

/* Log mode */
#ifdef __MSD_LOG_MODE_THREAD__
    #define MSD_LOG_MODE_THREAD         //线程模式
#elif defined __MSD_LOG_MODE_PROCESS__
    #define MSD_LOG_MODE_PROCESS        //进程模式
#endif

/* Multiplexing I/O mode */
#ifdef __MSD_EPOLL_MODE__
    #define MSD_EPOLL_MODE              //epoll  
#elif defined __MSD_SELECT_MODE__           
    #define MSD_SELECT_MODE             //select
#endif

/* ---------------PROJECT HEADER FILE----------- */
#include "msd_lock.h"
#include "msd_log.h"
#include "msd_string.h"
#include "msd_hash.h"
#include "msd_conf.h"
#include "msd_vector.h"
#include "msd_dlist.h"
#include "msd_ae.h"
#include "msd_so.h"
#include "msd_daemon.h"
#include "msd_anet.h"
#include "msd_thread.h"
#include "msd_master.h"
#include "msd_plugin.h"
#include "msd_signal.h"
 
/* -----------------PUBLIC MACRO---------------- */
#define MSD_OK       0
#define MSD_ERR     -1
#define MSD_FAILED  -2
#define MSD_NONEED  -3
#define MSD_END     -4


#define MSD_PROG_TITLE   "Mossad"
#define MSD_PROG_NAME    "mossad"
#define MOSSAD_VERSION   "0.0.1"

/* --------Structure--------- */

/* 包含了全部so的函数的容器，so函数由用户编写的 */
typedef struct msd_so_func_struct 
{
    int (*handle_init)(void *);                              /* 当进程初始化的时候均会调用，参数是全局变量g_conf，此函数可选 */
    int (*handle_worker_init)(void *, void*);                /* worker线程初始化的时候均会调用，参数是worker句柄，此函数可选 */
    int (*handle_last_preparation)(void *, void*);           /* 所有的worker就绪之后，可能会有最终的一些工作要做，比如再生成一个非worker线程的其他线程，此函数可选 */
    int (*handle_fini)(void *);                              /* 当进程退出的时候均会调用，参数是全局变量g_conf，此函数可选 */
    int (*handle_open)(msd_conn_client_t *);
    int (*handle_close)(msd_conn_client_t *, const char *);  /* 当关闭与某个client的连接的时候调用，第一个参数client ip, 第二个client port，此函数可选 */                                             
    int (*handle_prot_len)(msd_conn_client_t *);             /* 用来获取client发送请求消息的具体长度，即得到协议长度
                                                                       * mossad获取到此长度之后从接收缓冲区中读取相应长度的请求数据，交给handle_process来处理
                                                                       * 第一个参数是接收缓冲区，第二个接收缓冲区长度，第三个client ip, 第四个client port 
                                                                       */
    int (*handle_process)(msd_conn_client_t *);              /* Worker进程专用，用来根据client的输入，产生出输出，吐出数据
                                                                       * 参数:第一个是接收缓冲区，第二个是接收内容长度
                                                                       *      第三个是发送缓冲区，第四个是发送内容的长度引用
                                                                       *      第五个参数client ip, 第六个client port
                                                                       */
} msd_so_func_t;

/* mossad实例结构 */
typedef struct _msd_instance_t{
    msd_str_t           *pid_file;
    msd_str_t           *conf_path;
    msd_conf_t          *conf;
    //msd_log_t           *log;

    msd_master_t        *master;                 /* master线程句柄 */ 
    msd_thread_pool_t   *pool;                   /* worker线程池句柄 */ 
    msd_thread_signal_t *sig_worker;             /* signal worker线程句柄 */ 

    msd_str_t           *so_file;                /* so文件路径 */
    msd_so_func_t       *so_func;                /* 装载了全部so的函数的容器 */
    void                *so_handle;              /* so句柄指针 */

    msd_lock_t          *thread_woker_list_lock; /* woker list 锁 */
    msd_lock_t          *client_conn_vec_lock;   /* client vector 锁 */
}msd_instance_t;
#endif

