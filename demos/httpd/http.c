/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  echo.c
 * 
 * Description :  Mossad框架Http服务器示例
 *                实现msd_plugin.h中的函数，即可将用户逻辑嵌入mossad框架
 * 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/
#include "msd_core.h"

#define HTTP_METHOD_GET     0
#define HTTP_METHOD_PUT     1
#define HTTP_METHOD_POST    2
#define HTTP_METHOD_HEAD    3

#define RESPONSE_BUF_SIZE   4096

static char *g_doc_root;
static char *g_index_file;

/**
 * 功能: 初始化回调
 * 参数: @conf
 * 说明: 
 *       1. 可选函数
 * 返回:成功:0; 失败:-x
 **/
int msd_handle_init(void *conf) 
{
    g_doc_root   = msd_conf_get_str_value(conf, "docroot", "./");
    g_index_file = msd_conf_get_str_value(conf, "index", "index.html");

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
    //欢迎信息写入sendbuf
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
void msd_handle_close(msd_conn_client_t *client, const char *inf) 
{
    MSD_DEBUG_LOG("Connection from %s:%d closed", client->remote_ip, client->remote_port);
}
*/

/**
 * 功能: 动态约定mossad和client之间的通信协议长度，即mossad应该读取多少数据，算作一次请求
 * 参数: @client指针
 * 说明: 
 *       1. 必选函数
 *       2. 如果暂时无法确定协议长度，返回0，mossad会继续从client读取数据
 *       3. 如果返回-1，mossad会认为出现错误，关闭此连接
 *       3. 最简单的Http协议request包示例:
 *           GET /www/index.html HTTP/1.1 \r\n
 *           Host: www.w3.org \r\n
 *           \r\n
 *           request-body.....
 * 返回:成功:协议长度; 失败:
 **/
int msd_handle_prot_len(msd_conn_client_t *client) 
{
    /* At here, try to find the end of the http request. */
    int content_len;
    char *ptr;
    char *header_end;
    
    /* 找到http协议request包Header的末尾"\r\n\r\n" */
    if (!(header_end = strstr(client->recvbuf->buf, "\r\n\r\n"))) 
    {
        /* 没有找到末尾，则认为此包不完整，返回0，mossad会继续读取 */
        return 0;
    }
    header_end += 4;
    
    /* find content-length header */
	/* Http请求头如果没有content-length字段，则长度就是请求行和请求头以及后面的两个回车换行的长度之后 
	 * 否则就再加上content-length字段
	 */
    if ((ptr = strstr(client->recvbuf->buf, "Content-Length:"))) 
    {
        content_len = strtol(ptr + strlen("Content-Length:"), NULL, 10);
        if (content_len == 0) 
        {
            MSD_ERROR_LOG("Invalid http protocol: %s", client->recvbuf->buf);
            return MSD_ERR;
        }
        return header_end - client->recvbuf->buf + content_len;
    } 
    else 
    {
        return header_end - client->recvbuf->buf;
    }
}

/**
 * 功能: 主要的用户逻辑
 * 参数: @client指针
 * 说明: 
 *       1. 必选函数
 *       2. 每次从recvbuf中应该取得recv_prot_len长度的数据，作为一个完整请求
 *       3. 最简单的Http协议reponse包示例:
 *              HTTP/1.1 200 OK \r\n
 *              Content-Type: text/html \r\n
 *              Content-Length: 158 \r\n
 *              Server: Apache-Coyote/1.1 \r\n
 *              \r\n
 *              response-body.....
 * 返回:成功:0; 失败:-x
 *       MSD_OK: 成功，并保持连接继续
 *       MSD_END:成功，不在继续，mossad将response写回client后，自动关闭连接
 *       MSD_ERR:失败，mossad关闭连接
 **/
int msd_handle_process(msd_conn_client_t *client) 
{
    /* Parse request and generate response. */
    int n = 0;
    int file_size;
    char *ptr = client->recvbuf->buf;
    char *ptr2;
    char file[512] = {};
    char *buf;
    int fd;
    
    if (!strncmp(ptr, "GET", 3)) 
    {  
        /* Only support method GET */
        ptr += 4;
        while(*ptr == '/')
        {
            ptr++;
        }
    } 
    else 
    {
        //TODO unsupported methods
        return MSD_ERR;
    }

    if (!(ptr2 = strstr(ptr, "HTTP/"))) 
    {
        return MSD_ERR;
    }

    *--ptr2 = '\0';
    
    /* Generate filename accessed */
    if( ptr[strlen(ptr) - 1] == '/' )
    {
        snprintf(file, sizeof(file), "%s/%s%s", g_doc_root, ptr, g_index_file);
    }
    else
    {
        snprintf(file, sizeof(file), "%s/%s", g_doc_root, ptr);
    }
    
    fd = open(file, O_RDONLY);
    if (fd < 0) 
    {
        /* 404 error */
        MSD_ERROR_LOG("Open file [%s] failed, 404 Error returned", file);
        snprintf(file, sizeof(file), "%s/404.html", g_doc_root);
        fd = open(file, O_RDONLY);
        file_size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
        msd_str_sprintf(&(client->sendbuf), "HTTP/1.1 404 Not Found\r\nServer: mossad \r\n"
                    "Content-Length: %d\r\n\r\n",file_size);
    }
    else
    {
        MSD_INFO_LOG("Open file [%s]", file);
        file_size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET); 
        msd_str_sprintf(&(client->sendbuf), "HTTP/1.1 200 OK\r\nServer: mossad \r\n"
                    "Content-Length: %d\r\n\r\n",file_size); 
    }
    buf = (char *)calloc(1, file_size+1);
    ptr = buf;
    ptr2 = buf + file_size;
    while(( n = read(fd, ptr, ptr2 - ptr)) > 0)
    {
        ptr += n;
    }
    *ptr = '\0';
    
    //支持二进制文件
    //msd_str_cat(&(client->sendbuf), buf);
    msd_str_cat_len(&(client->sendbuf), buf, file_size);
    free(buf);
    close(fd);
    
    /*
    // 注册回写事件--放入框架内部实现
    worker = msd_get_worker(client->worker_id);
    if (msd_ae_create_file_event(worker->t_ael, client->fd, MSD_AE_WRITABLE,
                msd_write_to_client, client) == MSD_ERR) 
    {
        msd_close_client(client->idx, "create file event failed");
        MSD_ERROR_LOG("Create write file event failed for connection:%s:%d", client->remote_ip, client->remote_port);
        return MSD_ERR;
    }
    */
    return MSD_END;
}

/**
 * 功能: mossad关闭的时候，触发此回调
 * 参数: @cycle
 * 说明: 
 *       1. 可选函数
 **/
/*
void handle_fini(void *cycle) 
{

}
*/
