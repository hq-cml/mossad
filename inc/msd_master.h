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

       
/*
    char is_server;
    int listen_fd;
    struct sockaddr_in listen_addr;
    in_addr_t srv_addr;
    int srv_port;
    unsigned int start_time;
    int nonblock;
    int listen_queue_length;
    int tcp_send_buffer_size;
    int tcp_recv_buffer_size;
    int send_timeout;
    int tcp_reuse;
    int tcp_nodelay;
    struct event ev_accept;
    //void *(* on_data_callback)(void *);
    ProcessHandler_t on_data_callback;
    
    
    
    
    qbh_ae_event_loop       *ael;
    static int              listen_fd;
    static char             sock_error[QBH_ANET_ERR_LEN];
    static qbh_dlist        *clients; 
    static int              client_limit;   // client节点个数限制 
    static int              client_timeout; // client超时时间 
    static time_t           unix_clock;
    static pid_t            conn_pid;       // conn进程的id 
    static qbh_vector_t     *conn_vec;    // vector结构，元素是qbh_client_conn指针，用来做client失效判断的
                                           //它的索引是client连接过来的时候，conn所用的对应的fd 
*/                                                

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



    int     idx;           /* 位于client_vec中的位置 */
    int     worker_id;     /* 由哪个woker线程负责处理该链接的所有请求 */

    /*
    int id;
    int worker_id;
    int client_fd;
    long task_id;
    long sub_task_id;
    in_addr_t client_addr;
    int client_port;
    time_t conn_time;
    func_t handle_client;
    struct event event;
    std::vector<struct event *> ev_dns_vec;
    enum conn_states state;
    enum error_no err_no;
    enum conn_type type;
    int ev_flags;
 
    void * userData;

    int r_buf_arena_id;
    char *read_buffer;
    unsigned int rbuf_size;
    unsigned int need_read;
    unsigned int have_read;

    int w_buf_arena_id;
    char *__write_buffer;
    char *write_buffer;
    unsigned int wbuf_size;
    unsigned int __need_write;
    unsigned int need_write;
    unsigned int have_write;
    char client_hostname[MAX_HOST_NAME + 1];
    */
}msd_conn_client_t;

int msd_master_cycle();
void msd_close_client(int client_idx, const char *info);
int msd_master_destroy(msd_master_t *master);

#endif

