/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  echo.c
 * 
 * Description :  Mossad框架回显示例
 *                实现msd_plugin.h中的函数，即可将用户逻辑嵌入mossad框架
 * 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/

#include "msd_core.h"

/**
 * 功能: 初始化回调
 * 参数: @conf
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int msd_handle_init(void *conf) 
{
    MSD_INFO_LOG("Msd_handle_init is called!");
    return MSD_OK;
}

/**
 * 功能: client连接被accept后，触发此回调
 * 参数: @client指针
 * 说明: 
 *       1. 可选函数
 *       2. 一般可以写一些欢迎信息到client上去 
 * 返回:成功:0; 失败:-x
 **/
/*
int msd_handle_open(msd_conn_client_t *client) 
{
    int write_len;
    char buf[1024];
    sprintf(buf, "Hello, %s:%d, welcom to mossad!\n", client->remote_ip, client->remote_port);
    // 欢迎信息写入sendbuf 
    msd_str_cpy(&(client->sendbuf), buf);
    
    if((write_len = write(client->fd, client->sendbuf->buf, client->sendbuf->len))
        != client->sendbuf->len)
    {
        MSD_ERROR_LOG("Handle open error! IP:%s, Port:%d. Error:%s", client->remote_ip, client->remote_port, strerror(errno));
        //TODO，将write加入ae_loop
        return MSD_ERR;
    }
    return MSD_OK;
}
*/
/**
 * 功能: mossad断开client连接的时候，触发此回调
 * 参数: @client指针
 *       @info，放置关闭连接的原因
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
/*
int msd_handle_close(msd_conn_client_t *client, const char *info) 
{
    int write_len;
    char buf[1024];
    sprintf(buf, "Mossad close the connection! Reason:%s\n", info);
    //欢送信息写入sendbuf 
    msd_str_cpy(&(client->sendbuf), buf);
    
    if((write_len = write(client->fd, client->sendbuf->buf, client->sendbuf->len))
        != client->sendbuf->len)
    {
        MSD_ERROR_LOG("Handle close error! IP:%s, Port:%d. Error:%s", client->remote_ip, client->remote_port, strerror(errno));
        //TODO，将write加入ae_loop
        return MSD_ERR;
    }
    return MSD_OK;
}
*/
/**
 * 功能: 动态约定mossad和client之间的通信协议长度，即mossad应该读取多少数据，算作一次请求
 * 参数: @client指针
 * 说明: 
 *       1. 必选函数
 * 返回:成功:协议长度; 失败:
 **/
int msd_handle_prot_len(msd_conn_client_t *client) 
{
    return client->recvbuf->len;
}

/**
 * 功能: 主要的用户逻辑
 * 参数: @client指针
 * 说明: 
 *       1. 必选函数
 *       2. 每次从recvbuf中应该取得recv_prot_len长度的数据，作为一个完整请求
 * 返回:成功:0; 失败:-x
 *       MSD_OK: 成功，并保持连接继续
 *       MSD_END:成功，不在继续，mossad将response写回client后，自动关闭连接
 *       MSD_ERR:失败，mossad关闭连接
 **/
int msd_handle_process(msd_conn_client_t *client) 
{
    //msd_thread_worker_t *worker; 
    /* 回显信息写入sendbuf */
    msd_str_cat_len(&(client->sendbuf), client->recvbuf->buf, client->recv_prot_len);
    
    /*
    if((write_len = write(client->fd, client->sendbuf->buf, client->sendbuf->len))
        != client->sendbuf->len)
    {
        MSD_ERROR_LOG("Handle process error! IP:%s, Port:%d. Error:%s", client->remote_ip, client->remote_port, strerror(errno));
        //TODO，将write加入ae_loop
        return MSD_ERR;
    }
    */
    
    /*
    worker = msd_get_worker(client->worker_id);
    // 注册回写事件，交由框架处理
    if (msd_ae_create_file_event(worker->t_ael, client->fd, MSD_AE_WRITABLE,
                msd_write_to_client, client) == MSD_ERR) 
    {
        msd_close_client(client->idx, "create file event failed");
        MSD_ERROR_LOG("Create write file event failed for connection:%s:%d", client->remote_ip, client->remote_port);
        return MSD_ERR;
    }
    */
    return MSD_OK;
}

/**
 * 功能: mossad关闭的时候，触发此回调
 * 参数: @cycle
 * 说明: 
 *       1. 可选函数
 **/
int msd_handle_fini(void *cycle) 
{
    MSD_INFO_LOG("Msd_handle_fini is called!");
    return MSD_OK;
}
