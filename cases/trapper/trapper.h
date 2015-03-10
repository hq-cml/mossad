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
#ifndef __TRAPPER__
#define __TRAPPER__
#include "msd_core.h"
#include "cJSON.h"
#include <curl/curl.h>
#include <float.h>
#include <limits.h>

#define TOTAL_BACK_END_CNT        10
#define BACK_END_LIST_MAX_LENGTH  10

typedef enum tos_back_stat{
    B_OK,
    B_BAD
}back_stat_t;

typedef struct tos_addr{
    char    *back_ip;    /* back_end ip */
    int      back_port;   /* back_end port */
}tos_addr_t;

typedef struct back_end{
    int               fd;           /* 主fd */
    int               idx;          /* 当前fd的在链表中的索引 */
    back_stat_t   status;
    time_t            access_time;
    msd_str_t         *recvbuf;
    msd_str_t         *sendbuf;
    msd_dlist_t       *address_list; /* tos_addr_t链表 */
}back_end_t;

/* worker私有数据 */
typedef struct trap_worker_data{
    msd_dlist_t         *back_end_list;           /* 向后发送结果数据的地址(transfer) */
    msd_str_t           *host_game_map_intfs;
    msd_hash_t          *host_game_hash;          /* 机器=>业务的映射关系hash */
    int                  back_end_alive_interval; /* 后端存活状态探测频率 */
    int                  host_game_interval;      /* 机器=>业务的映射关系更新频率 */
    int                  cal_send_back_interval;  /* 定期计算时间窗口内的数值，然后将计算结果向后发送 */
    msd_hash_t          *item_to_clu_item_hash;   /* item_id=>cluster_item_id_list映射关系 */
    msd_hash_t          *item_value_hash;         /* 所有值需要计算集群监控的值的集合: game=>host_list */
    msd_thread_worker_t *worker;                  /* worker_data所依附的worker句柄 */
}trap_worker_data_t;

typedef struct item_value
{
    char        hostname[128]; /* 主机名 */
    msd_str_t   *value_str;    /* 值串 */
    time_t      time;          /* 最新时间 */
}item_value_t;

typedef struct host_value
{
    char   hostname[128];   /* 主机名 */
    float  value;           /* 值 */
}host_value_t;  

typedef struct cal_struct
{
    void *worker_data;   /* 主机名 */
    void *game;           /* 值 */
}cal_struct_t;  


back_end_t* deal_one_back_line(msd_conf_t *conf, const char *back_end_name, msd_thread_worker_t *worker);
int chose_one_avail_fd(back_end_t *back_end, msd_thread_worker_t *worker);
int check_fd_cron(msd_ae_event_loop *el, long long id, void *privdate);
int update_host_game_cron(msd_ae_event_loop *el, long long id, void *privdate);
void send_to_back(msd_ae_event_loop *el, int fd, void *privdata, int mask);
void read_from_back(msd_ae_event_loop *el, int fd, void *privdata, int mask);
int update_host_game_map(char *url, msd_hash_t *hash);
int _hash_deal_with_item_foreach(const msd_hash_entry_t *he, void *userptr);
void *_hash_val_set_item_id(const void *item_id_list);
void _hash_val_free_item_id(void *dlist);
void *_hash_val_set_item_value(const void *game_name);
void _hash_val_free_item_value(void *dlist);
void _hash_val_free_host_value(void *dlist);
msd_dlist_node_t *item_value_list_search(msd_dlist_t *dl, char *hostname);
msd_dlist_node_t *host_value_list_search(msd_dlist_t *dl, char *hostname);
item_value_t *item_value_node_init(char *hostname) ;
int cal_send_back_cron(msd_ae_event_loop *el, long long id, void *privdate);
void _free_node_item_value(void *ptr);
void _free_node_host_value(void *ptr);
int _hash_delete_foreach(const msd_hash_entry_t *he, void *ht);


#endif
