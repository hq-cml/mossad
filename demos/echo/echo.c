/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  echo.c
 * 
 * Description :  Mossad框架回显示例
 *                实现msd_plugin.h中的函数，即可将用户逻辑嵌入mossad框架
 * 
 *     Created :  May 6, 2012 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
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
    MSD_DEBUG_LOG("Msd_handle_init is called!");
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
int msd_handle_open(msd_conn_client_t *client) 
{
    int write_len;
    char buf[1024];
    sprintf(buf, "Hello, %s:%d, welcom to mossad!\n", client->remote_ip, client->remote_port);
    /* 欢迎信息写入sendbuf */
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

/**
 * 功能: mossad断开client连接的时候，触发此回调
 * 参数: @client指针
 *       @info，放置关闭连接的原因
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
void msd_handle_close(msd_conn_client_t *client, const char *info) 
{
    int write_len;
    char buf[1024];
    sprintf(buf, "Mossad close the connection! Reason:%s\n", info);
    /* 欢迎信息写入sendbuf */
    msd_str_cpy(&(client->sendbuf), buf);
    
    if((write_len = write(client->fd, client->sendbuf->buf, client->sendbuf->len))
        != client->sendbuf->len)
    {
        MSD_ERROR_LOG("Handle close error! IP:%s, Port:%d. Error:%s", client->remote_ip, client->remote_port, strerror(errno));
        //TODO，将write加入ae_loop
    }
    return;
}

/**
 * 功能: 动态约定mossad和client之间的通信协议长度，即mossad应该读取多少数据，算作一次请求
 * 参数: @client指针
 * 说明: 
 *       1. 必选函数
 * 返回:成功:协议长度; 失败:
 **/
int msd_handle_input(msd_conn_client_t *client) 
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
 **/
int msd_handle_process(msd_conn_client_t *client) 
{
    int write_len;
    //TODO 参考qbench是怎么write的
    /* 回显信息写入sendbuf */
    msd_str_cpy_len(&(client->sendbuf), client->recvbuf->buf, client->recv_prot_len);

    if((write_len = write(client->fd, client->sendbuf->buf, client->sendbuf->len))
        != client->sendbuf->len)
    {
        MSD_ERROR_LOG("Handle process error! IP:%s, Port:%d. Error:%s", client->remote_ip, client->remote_port, strerror(errno));
        //TODO，将write加入ae_loop
        return MSD_ERR;
    }
    return 0;
}

/**
 * 功能: mossad关闭的时候，触发此回调
 * 参数: @cycle
 * 说明: 
 *       1. 可选函数
 **/
void msd_handle_fini(void *cycle) 
{
    MSD_DEBUG_LOG("Msd_handle_fini is called!");
}
