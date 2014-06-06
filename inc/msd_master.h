/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_master.h
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

#ifndef __MSD_MASTER_H_INCLUDED__ 
#define __MSD_MASTER_H_INCLUDED__


#define MSD_MSG_MAGIC      0x567890EF
#define MSD_MAGIC_DEBUG    0x1234ABCD

typedef enum msd_master_stat{
    M_RUNNING,
    M_STOPING,
    M_STOP
}msd_master_stat_t;

typedef enum msd_client_stat{
    C_COMMING,                  /* 连接刚来到 */
    C_DISPATCHING,              /* 连接正在分配中 */
    C_WAITING,                  /* 连接已经分配到了具体woker中，等待request到来 */
    C_RECEIVING,                /* request到来，但是未凑足一个完整的协议包，继续等待 */
    C_PROCESSING,               /* request长度足够完整，开始处理 */
    C_CLOSING                   /* 连接关闭中 */
}msd_client_stat_t;


typedef struct msd_master
{
    int                 listen_fd;
    int                 client_limit;   /* client节点个数限制 */
    int                 poll_interval;  /* Master线程Cron频率 */
    msd_master_stat_t   status;         /* Master线程状态 */
    
    int                 cur_conn;       /* 上一次新增client在client_vec的位置 */
    int                 cur_thread;     /* 上一次新增client时分配的线程号 */
    int                 total_clients;  /* 当前client的总个数 */
    unsigned int        start_time;
    msd_vector_t        *client_vec;    /* msd_conn_client类型的vector，新的连接，会找到自己的合适位置 */
    msd_ae_event_loop   *m_ael;         /* ae句柄，用于listen_fd和signal_notify的监听 */
}msd_master_t;

typedef struct msd_conn_client
{
    int     magic;         /* 一个初始化的时候置上的魔幻数，用来标识cli指针是否发生异常 */
    int     fd;            /* client连接过来之后，conn这边生成的fd (非阻塞的!) */    
    int     close_conn;    /* whether close connection after send response */
    char    *remote_ip;    /* client ip */
    int     remote_port;   /* client port */
    int     recv_prot_len; /* 服务端和客户端通信协议规定好的读取长度 */
    msd_str_t  *sendbuf;      /* 发送缓冲 */
    msd_str_t  *recvbuf;      /* 接收缓冲 */
    time_t  access_time;   /* client最近一次来临时间，包括:创建连接、读取、写入 */
    msd_client_stat_t status; /* client的状态 */


    int     idx;           /* 位于client_vec中的位置 */
    int     worker_id;     /* 由哪个woker线程负责处理该链接的所有请求 */
}msd_conn_client_t;

int msd_master_cycle();
void msd_close_client(int client_idx, const char *info);
int msd_master_destroy(msd_master_t *master);

#endif

