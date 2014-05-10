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
 *                依附于mossad框架的服务器程序，需要实现此头文件下的
 *                函数，真正的用户逻辑在这6个回调函数中体现。
 *                此6个函数，会在适当的时机，进行调用。
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
#include <sys/cdefs.h>
#include "msd_core.h"

__BEGIN_DECLS

/* It's optional. If implemented, it would be invoked when
 * the process at beginning phase. You should do some 
 * initializetion work in it. When success, please ruturn
 * 0, otherwise -1 should be returned. Upon failure, the
 * verben daemon will exit immediately. */
int msd_handle_init(void *cycle, int proc_type);

/* It's optional. If implemented, it would be invoked when
 * the process would exit. You should do some destructation
 * work in it. */
void msd_handle_fini(void *cycle, int proc_type);

/* It's optional.  When a connection accepted, the `handle_open' 
 * would be invoked. The `sendbuf' and 'len' can be used to write
 * some message to client upon connection. The implementation of 
 * this function should allocate memory from heap, and return
 * the address in `sendbuf' and its length in 'len'. Otherwise,
 * set `sendbuf' to NULL. When success, please ruturn 0, otherwise,
 * the connection will be closed. */
//int msd_handle_open(char **sendbuf, int *len, const char *remote_ip, int port);
int msd_handle_open(void *cycle);

/* It's optinal. When a connection closed, it would be invoked. */
void msd_handle_close(const char *remote_ip, int port);

/* This function is mandatory. Your plugin MUST implemente this 
 * function. This function should return the length of a protocal
 * message. When it's unknown, return 0. You also can return -1
 * to close this connection. */
int msd_handle_input(char*recvbuf, int recvlen, 
        const char *remote_ip, int port);

/* This function is mandatory. Your plugin MUST implement it. 
 * The protocol message mainly was processed in it. */
int msd_handle_process(char *recvbuf, int recvlen, 
        char **sendbuf, int *sendlen, const char *remote_ip, int port);

__END_DECLS
#endif /* __MSD_PLUGIN_H_INCLUDED__ */
