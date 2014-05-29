/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_anet.c 
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
#include "msd_core.h"

/**
 * 功能: 格式化err缓冲区，不定参
 * 参数: @err, @fmt
 **/
static void msd_anet_set_error(char *err, const char *fmt, ...) 
{
    va_list ap;
    if (!err) 
    {
        return;
    }

    va_start(ap, fmt);
    vsnprintf(err, MSD_ANET_ERR_LEN, fmt, ap);
    va_end(ap);
}

/**
 * 功能: 创建TCP socket
 * 参数: @err, 错误输出的缓冲
 *       @domain, 协议域
 * 返回: 成功，新生成的fd，失败，-x
 **/
static int msd_anet_create_socket(char *err, int domain) 
{
    int s, on = 1;
    int flags;
    if ((s = socket(domain, SOCK_STREAM, 0)) == -1) 
    {
        msd_anet_set_error(err, "Create socket failed: %s", strerror(errno));
        return MSD_ERR;
    }

    /* 允许套接口和一个已在使用中的地址捆绑 */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) 
    {
        msd_anet_set_error(err, "Setsockopt SO_REUSEADDR: %s", strerror(errno));
        return MSD_ERR;
    }
    
    /* 设置CLOEXEC，当exec时释放此fd */
    if ((flags = fcntl(s, F_GETFD)) < 0)
    {
        msd_anet_set_error(err, "Get fcntl failed: %s", strerror(errno));
        return MSD_ERR;        
    }
    else
    {
        if(fcntl(s, F_SETFD, flags | FD_CLOEXEC)<0)
        {
            msd_anet_set_error(err, "Set close-on-exec failed: %s", strerror(errno));
            return MSD_ERR;            
        }
    }
    
    return s;
}

/**
 * 功能: 先bind后listen
 * 参数: @err,@sockfd,@sa,地址结构,@len,sa长度
 * 返回: 成功，失败，
 **/
static int msd_anet_bind_listen(char *err, int sockfd, struct sockaddr *sa, socklen_t len) 
{
    if (bind(sockfd, sa, len) == -1) 
    {
        msd_anet_set_error(err, "bind failed: %s", strerror(errno));
        close(sockfd);
        return MSD_ERR;
    }

    /* The magic 511 constant is from nginx. 
     * Use a backlog of 512 entries. We pass 511 to the listen() call because
     * the kernel does: backlogsize = roundup_pow_of_two(backlogsize + 1);
     * which will thus give us a backlog of 512 entries */
    if (listen(sockfd, 511) == -1) 
    {
        msd_anet_set_error(err, "listen failed:%s", strerror(errno));
        close(sockfd);
        return MSD_ERR;
    }
    return MSD_OK;
}

/**
 * 功能: Set the socket nonblocking.
 * 参数: @err, @fd
 * 描述:
 *      1. Note that fcntl(2) for F_GETFL and F_SETFL can't be interrupted by a signal.
 *      2. 对于非阻塞的fd进行accept，会立刻退出，但是由于一般服务器都会利用I/O复用技术
 *      3. 对于accept的检测的fd，无论其是否阻塞，效果没有差异，但为了防止异常情况，比如
 *         select返回了，在调用accept之前，如果连接被异常终止，这时 accept 调用可能会由
 *         于没有已完成的连接而阻塞，直到有新连接建立。
 *      4. 对于accept返回的fd，则应该设置为非阻塞，防止处理线程阻塞于某个I/O操作，即使
 *         使用了I/O复用，也需要非阻塞。因为，I/O复用只能说明 socket 可读或者可写，不能
 *         说明能读入或者能写出多少数据。比如，socket 的写缓冲区有10个字节的空闲空间，
 *         这时监视的 select 返回，然后在该 socket 上进行写操作。但是如果要写入100字节，
 *         如果 socket 没有设置非阻塞，调用 write 就会阻塞在那里。在多个 socket 的情况下
 *         ，读写一个socket 时阻塞，会影响到其他的 socket 。
 * 返回: 成功，0，失败，-x
 **/
int msd_anet_nonblock(char *err, int fd) 
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1) 
    {
        msd_anet_set_error(err, "fcntl(F_GETFL) failed: %s", strerror(errno));
        return MSD_ERR;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        msd_anet_set_error(err, "fcntl(F_SETFL, O_NONBLOCK) failed: %s", strerror(errno));
        return MSD_ERR;
    }
    return MSD_OK;
}

/**
 * 功能: 设置no_delay
 * 参数: @err, @fd
 * 描述:
 *      1. 关闭禁止Nagle算法，即不等零碎组包，对网络性能影响较大
 * 返回: 成功，0，失败，-x
 **/
int msd_anet_tcp_nodelay(char *err, int fd) 
{
    int yes = 1;

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1) 
    {
        msd_anet_set_error(err, "setsockopt TCP_NODELAY failed: %s", strerror(errno));
        return MSD_ERR;
    }
    return MSD_OK;
}

/**
 * 功能: 设置缓冲曲大小
 * 参数: @err, @fd, @send_buffsize, @recv_buffsize
 * 描述:
 *      1. 不支持将缓冲区设置为0
 *      2. 对于侦听fd，没必要设置缓冲区；对于各已存在的连接fd，可以设置缓冲区
 * 返回: 成功，0,失败，-x
 **/
int msd_anet_set_buffer(char *err, int fd, int send_buffsize, int recv_buffsize) 
{
    if(!send_buffsize)
    {
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &send_buffsize, sizeof(send_buffsize)) == -1) 
        {
            msd_anet_set_error(err, "setsockopt SO_SNDBUF failed: %s", strerror(errno));
            return MSD_ERR;
        }
    }
    
    if(!recv_buffsize)
    {
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &recv_buffsize, sizeof(recv_buffsize)) == -1) 
        {
            msd_anet_set_error(err, "setsockopt SO_RCVBUF failed: %s", strerror(errno));
            return MSD_ERR;
        }
    }
    return MSD_OK;
}

/**
 * 功能: 设置keepalive
 * 参数: @err, @fd
 * 描述:
 *      1. 
 * 返回: 成功，失败，
 **/
int msd_anet_tcp_keepalive(char *err, int fd) 
{
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) 
    {
        msd_anet_set_error(err, "setsockopt SO_KEEPALIVE failed: %s", strerror(errno));
        return MSD_ERR;
    }
    return MSD_OK;
}

/**
 * 功能: 建立TCP服务器: create->bind->listen
 * 参数: @err, @addr, @port
 * 描述:
 *      1. 如果addr为空，则绑定全部地址
 * 返回: 成功，sockfd, 失败，-x
 **/
int msd_anet_tcp_server(char *err, char *addr, int port) 
{
    int sockfd;
    struct sockaddr_in sa;
    
    /* 创建AF_INET */
    if ((sockfd = msd_anet_create_socket(err, AF_INET)) == MSD_ERR) 
    {
        return MSD_ERR;
    }
     
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY); /* 绑定所有网卡 */
    
    /* 如果addr非空，则bind指定的ip地址 */
    if (addr && inet_aton(addr, &sa.sin_addr) == 0) 
    {
        msd_anet_set_error(err, "invalid bind address");
        close(sockfd);
        return MSD_ERR;
    }

    if (msd_anet_bind_listen(err, sockfd, (struct sockaddr*)&sa, sizeof(sa)) == MSD_ERR) 
    {
        return MSD_ERR;
    }
    return sockfd;
}

/**
 * 功能: 通用accept
 * 参数: @err, sockfd, sa, len
 * 描述:
 *      1. sa/len--值/结果参数 
 *      2. 阻塞过程中，若被信号中断，则继续
 * 返回: 成功，新的fd，失败，-x
 **/
static int msd_anet_generic_accept(char *err, int sockfd, struct sockaddr *sa, socklen_t *len) 
{
    int fd;
    while (1) 
    {
        fd = accept(sockfd, sa, len);
        if (fd < 0) 
        {
            /* 阻塞过程中，若被信号中断，则继续 */
            if (errno == EINTR) 
            {
                continue;
            } 
            else 
            {
                msd_anet_set_error(err, "accept failed:%s, errno:%d, fd:%d", strerror(errno),errno, fd);
                return MSD_ERR;
            }
        }
        break;
    }
    return fd;
}

/**
 * 功能: Tcp accept
 * 参数: @err, sockfd, ip, port
 * 描述:
 *      1. 将ip和port存入参数所指定地址
 * 返回: 成功，新fd, 失败，-x
 **/
int msd_anet_tcp_accept(char *err, int sockfd, char *ip, int *port) 
{
    int fd;
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);
    if ((fd = msd_anet_generic_accept(err, sockfd, (struct sockaddr*)&sa, &salen)) == MSD_ERR) 
    {
        return MSD_ERR;
    }

    /* Set the ip and port*/
    if (ip) 
    {
        strcpy(ip, inet_ntoa(sa.sin_addr));
    }
    if (port) 
    {
        *port = ntohs(sa.sin_port);
    }
    return fd;
}

/**
 * 功能: 向fd读取count个字节，存储于buf
 * 参数: @
 * 描述:
 *      1. 如果遇到EOF，或者出错，则返回
 *      2. 一直读取，直到读够了count字节
 * 返回: 成功，一共读取的字符数，失败，-x
 **/
int msd_anet_read(int fd, char *buf, int count) 
{
    int nread, totlen = 0;
    while (totlen < count) 
    {
        nread = read(fd, buf, count - totlen);
        if (nread == 0) 
        {
            return totlen; /* EOF */
        }

        if (nread < 0) 
        {
            return MSD_ERR;
        }
        totlen += nread;
        buf += nread;
    }
    return totlen;
}

/**
 * 功能: 向fd写入count个字节
 * 参数: @
 * 描述:
 *      1. 如果遇到错误，则退出
 *      2. 一直写，直到写满了count个字符
 * 返回: 成功，失败，
 **/
int msd_anet_write(int fd, char *buf, int count) 
{
    int nwritten, totlen = 0;
    while (totlen < count) 
    {
        nwritten = write(fd, buf, count - totlen);
        if (nwritten == 0) 
        {
            return totlen;
        }
        if (nwritten < 0) 
        {
            return MSD_ERR;
        }
        totlen += nwritten;
        buf += nwritten;
    }
    return totlen;
}

/**
 * 功能: resolve
 * 参数: @err, host, ipbuf
 * 描述:
 *      1. 对参数host进行dns解析，将结果存入ipbuf
 * 返回: 成功，0, 失败，-x
 **/
int msd_anet_resolve(char *err, char *host, char *ipbuf) 
{
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    if (inet_aton(host, &sa.sin_addr) == 0) 
    {
        struct hostent *he;
        he = gethostbyname(host);
        if (he == NULL) 
        {
            msd_anet_set_error(err, "resolve %s failed: %s", host, hstrerror(h_errno));
            return MSD_ERR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }
    strcpy(ipbuf, inet_ntoa(sa.sin_addr));
    return MSD_OK;
}

/**
 * 功能: 根据fd获取到对端信息，存入参数所指定地址
 * 参数: @err, fd, ip, port
 * 返回: 成功，失败，
 **/
int msd_anet_peer_tostring(char *err, int fd, char *ip, int *port) 
{
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);

    if (getpeername(fd, (struct sockaddr*)&sa, &salen) == -1) 
    {
        msd_anet_set_error(err, "getpeername failed:%s", strerror(errno));
        return MSD_ERR;
    }

    if (ip) 
    {
        strcpy(ip, inet_ntoa(sa.sin_addr));
    }

    if (port) 
    {
        *port = ntohs(sa.sin_port);
    }
    return MSD_OK;
}

/**
 * 功能: 通用连接
 * 参数: @err, addr, port, flags
 * 返回: 成功，新的fd；失败，-x
 **/
static int msd_anet_tcp_generic_connect(char *err, char *addr, int port, int flags) 
{
    int sockfd;
    struct sockaddr_in sa;

    if ((sockfd = msd_anet_create_socket(err, AF_INET)) == MSD_ERR) 
    {
        MSD_ERROR_LOG("Msd_anet_create_socket failed:%s", err);
        return MSD_ERR;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_aton(addr, &sa.sin_addr) == 0) 
    {
        struct hostent *he;
        he = gethostbyname(addr);
        if (he == NULL) 
        {
            msd_anet_set_error(err, "resolve %s failed: %s", addr, hstrerror(h_errno));
            MSD_ERROR_LOG("Gethostbyname failed:%s", err);
            close(sockfd);
            return MSD_ERR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }

    if (flags & MSD_ANET_CONNECT_NONBLOCK) 
    {
        if (msd_anet_nonblock(err, sockfd) != MSD_OK) 
        {
            return MSD_ERR;
        }
    }

    /* 当以非阻塞的方式来进行连接的时候，返回的结果如果是 -1,这并不代表这次连接
     * 发生了错误，如果它的返回结果是 EINPROGRESS，那么就代表连接还在进行中。 
     * 后面可以通过poll或者select来判断socket是否可写，如果可以写，说明连接完成了
     **/
    if (connect(sockfd, (struct sockaddr*)&sa, sizeof(sa)) == -1) 
    {
        if (errno == EINPROGRESS && flags & MSD_ANET_CONNECT_NONBLOCK) {
            return sockfd;
        }
        msd_anet_set_error(err, "connect %s failed: %s", addr, strerror(errno));
        MSD_ERROR_LOG("Connect failed:%s", err);
        close(sockfd);
        return MSD_ERR;
    }
    return sockfd;
}

/**
 * 功能: 阻塞连接
 **/
int msd_anet_tcp_connect(char *err, char *addr, int port) 
{
    return msd_anet_tcp_generic_connect(err, addr, port, MSD_ANET_CONNECT);
}

/**
 * 功能: 非阻塞连接
 * 说明：
 *       TCP socket 被设为非阻塞后调用 connect ，connect函数会立即返回 EINPROCESS，但 
 *       TCP 的 3 次握手继续进行。之后可以用 select 检查 连接是否建立成功。
 *       非阻塞 connect 有3 种用途：
 *          1. 在3 次握手的同时做一些其他的处理。
 *          2. 可以同时建立多个连接。
 *　　　　　3. 在利用 select 等待的时候，可以给 select 设定一个时间，
 *             从而可以缩短 connect 的超时时间。
 **/
int msd_anet_tcp_nonblock_connect(char *err, char *addr, int port) 
{
    return msd_anet_tcp_generic_connect(err, addr, port,MSD_ANET_CONNECT_NONBLOCK);
}

#ifdef __MSD_ANET_TEST_MAIN__
int main(void) 
{
    fd_set rfds;
    char error[MSD_ANET_ERR_LEN];
    char msg[] = "hello,world\r\n";
    char ip[INET_ADDRSTRLEN];
    struct timeval timeout;
    int ready;
    int port;
    int fd;
    int sock;

    if (msd_anet_resolve(error, "localhost", ip) == MSD_ERR) 
    {
        fprintf(stderr, "%s\n", error);
        exit(1);
    }
    printf("LOCAL:%s\n", ip);

    if ((fd = msd_anet_tcp_server(error, ip, 8888)) == MSD_ERR) 
    {
        fprintf(stderr, "%s\n", error);
        exit(1);
    }
    
    msd_anet_tcp_nodelay(error, fd);
    /* 对于accpet的对象，也有必要设置非阻塞 */
    msd_anet_nonblock(error, fd);
    
    /*
    //测试NONBLOCK和NODELAY
    msd_anet_nonblock(error, fd);
    msd_anet_tcp_nodelay(error, fd);
    if((sock = msd_anet_tcp_accept(error, fd, ip, &port)) != MSD_ERR)
    {
        printf("accept  =>%s:%d\n", ip, port);
        write(sock, msg, strlen(msg));
        close(sock);  
    }
    else
    {
        fprintf(stderr, "%s\n", error);
    }    
    exit(0);
    */
    
    while (1) 
    {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        if ((ready = select(fd + 1, &rfds, NULL, NULL, &timeout)) == -1 && errno != EINTR) 
        {
            perror("select");
            exit(1);
        }
        
        if (ready > 0) 
        {
            if (FD_ISSET(fd, &rfds)) 
            {
                if((sock = msd_anet_tcp_accept(error, fd, ip, &port)) != MSD_ERR)
                {
                    printf("accept  =>%s:%d\n", ip, port);
                    
                    if (msd_anet_peer_tostring(error, sock, ip, &port) == MSD_ERR) 
                    {
                        fprintf(stderr, "%s\n", error);
                        continue;
                    }
                    printf("get peer=>%s:%d\n\n", ip, port);
                    msd_anet_read(sock, msg, 5);
                    msd_anet_write(sock, msg, strlen(msg));
                    close(sock);                    
                }

            }
        }
    }
    exit(0);
}
#endif /* __MSD_ANET_TEST_MAIN__ */

