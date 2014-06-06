/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_plugin.h
 * 
 * Description :  Msd_plugin. 
 *                依附于mossad框架的服务器程序，用户只需要实现此头文件下的
 *                6个函数，此6个函数，会在适当的时机，进行调用。
 * 
 *     Created :  May 6, 2012 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/

#ifndef __MSD_PLUGIN_H_INCLUDED__
#define __MSD_PLUGIN_H_INCLUDED__
/*
#include <sys/cdefs.h>
#include "msd_core.h"
*/
__BEGIN_DECLS

/**
 * 功能: 初始化回调
 * 参数: @conf
 * 说明: 
 *       1. 可选函数
 *       2. mossad初始化阶段，调用此函数，可以做一些初始化工作
 *       3. 如果此函数失败，mossad会直接退出
 * 返回:成功:0; 失败:-x
 **/
int msd_handle_init(void *conf);

/**
 * 功能: mossad关闭的时候，触发此回调
 * 参数: @cycle
 * 说明: 
 *       1. 可选函数
 *       2. 此函数内部可以做一些销毁工作
 **/
int msd_handle_fini(void *cycle);

/**
 * 功能: client连接被accept后，触发此回调
 * 参数: @client指针
 * 说明: 
 *       1. 可选函数
 *       2. 一般可以写一些欢迎信息到client上去 
 * 返回:成功:0; 失败:-x
 **/
int msd_handle_open(msd_conn_client_t *client);

/**
 * 功能: mossad断开client连接的时候，触发此回调
 * 参数: @client指针
 *       @info，放置关闭连接的原因
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int msd_handle_close(msd_conn_client_t *client, const char *info);

/**
 * 功能: 动态约定mossad和client之间的通信协议长度
 *       即mossad应该从client->recvbuf中读取多少数据，算作一次完整请求
 * 参数: @client指针
 * 说明: 
 *       1. 必选函数!
 *       2. 如果暂时无法确定协议长度，返回0，mossad会继续从client读取数据
 *       3. 如果返回-1，mossad会认为出现错误，关闭此链接
 * 返回:成功:协议长度; 失败:
 **/
int msd_handle_prot_len(msd_conn_client_t *client);

/**
 * 功能: 主要的用户逻辑
 * 参数: @client指针
 * 说明: 
 *       1. 必选函数
 *       2. 每次从recvbuf中应该取得recv_prot_len长度的数据，作为一个完整请求
 *       3. 将处理完毕的结果，放入sendbuf，然后发送回去(注册ae写函数，由框架发送回去为宜)
 * 返回:成功:MSD_OK, MSD_END; 失败:-x
 *       MSD_OK: 成功，并保持连接继续
 *       MSD_END:成功，不在继续，mossad将response写回client之后，自动关闭连接
 *       MSD_ERR:失败，mossad关闭连接
 **/
int msd_handle_process(msd_conn_client_t *client);

__END_DECLS
#endif /* __MSD_PLUGIN_H_INCLUDED__ */

