/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_anet.h 
 * 
 * Description :  Msd_anet, Basic TCP socket stuff. Derived from redis.
 * 
 *     Created :  May 19, 2012 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/
#ifndef __MSD_ANET_H_INCLUDED__
#define __MSD_ANET_H_INCLUDED__

/*
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
      
#define MSD_OK      0
#define MSD_ERR     -1
*/

#define MSD_ANET_ERR_LEN    256
#define MSD_ANET_BUF_SIZE   256*1024 /* 256K */

#define MSD_ANET_CONNECT            0
#define MSD_ANET_CONNECT_NONBLOCK   1

int msd_anet_tcp_connect(char *err, char *addr, int port);
int msd_anet_tcp_nonblock_connect(char *err, char *addr, int port);
int msd_anet_read(int fd, char *buf, int count);
int msd_anet_resolve(char *err, char *host, char *ipbuf);
int msd_anet_tcp_server(char *err, char *bindaddr, int port);
int msd_anet_tcp_accept(char *err, int serversock, char *ip, int *port);
int msd_anet_write(int fd, char *buf, int count);
int msd_anet_nonblock(char *err, int fd);
int msd_anet_tcp_nodelay(char *err, int fd);
int msd_anet_tcp_keepalive(char *err, int fd);
int msd_anet_peer_tostring(char *err, int fd, char *ip, int *port);
int msd_anet_set_send_buffer(char *err, int fd, int buffsize);

#endif /* __MSD_ANET_H_INCLUDED__ */
