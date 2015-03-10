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

#include "trapper.h"

/*
 *返回: 成功，一个后端句柄;失败:NULL
 */
back_end_t* deal_one_back_line(msd_conf_t *conf, const char *back_end_name, msd_thread_worker_t *worker)
{
    int j, cnt;
    char back_end_buf[1024];
    char *back_end_n;
    unsigned char *back_arr[BACK_END_LIST_MAX_LENGTH];
    unsigned char *ip_port[BACK_END_LIST_MAX_LENGTH];
    tos_addr_t *tmp;
    back_end_t *back_end;
    
    if(!(back_end = malloc(sizeof(back_end_t))))
    {
        MSD_ERROR_LOG("Bad fomat back address list!");
        return NULL;
    }
    back_end->fd          = -1;
    back_end->idx         = -1;
    back_end->status      = B_BAD;
    back_end->recvbuf     = msd_str_new_empty(); 
    back_end->sendbuf     = msd_str_new_empty(); 
    back_end->access_time = time(NULL);
    if(!(back_end->address_list = msd_dlist_init()))
    {
        msd_str_free(back_end->recvbuf);
        msd_str_free(back_end->sendbuf);
        free(back_end);
        MSD_ERROR_LOG("msd_dlist_init failed!");
        return NULL;        
    }
    
    back_end_n = msd_conf_get_str_value(conf, back_end_name, NULL);
    if(back_end_n)
    {
        memset(back_end_buf,  0, 1024);
        strncpy(back_end_buf, back_end_n, 1024);
        cnt = msd_str_explode((unsigned char *)back_end_buf, back_arr, 
                        BACK_END_LIST_MAX_LENGTH, (const unsigned char *)";");
        if(MSD_ERR == cnt)
        {
            MSD_ERROR_LOG("Bad fomat back address list!");
            goto deal_one_err;
        }
          
        for(j=0; j<cnt; j++)
        {
            if(2 != msd_str_explode(back_arr[j], ip_port, 2, (const unsigned char *)":"))
            {
                MSD_ERROR_LOG("Error fomat back address!");
                goto deal_one_err;
            }

            if(!(tmp = malloc(sizeof(tos_addr_t))))
            {
                MSD_ERROR_LOG("Malloc failed!");
                goto deal_one_err;
            }
            
            if(!(tmp->back_ip   = strdup((const char *)ip_port[0])))
            {
                MSD_ERROR_LOG("Strdup failed!");
                goto deal_one_err;
            }
            tmp->back_port = atoi((const char *)ip_port[1]);
            if(!msd_dlist_add_node_tail(back_end->address_list, tmp))
            {
                MSD_ERROR_LOG("Msd_dlist_add_node_tail failed!");
                goto deal_one_err;
            }
            
        }
        
        if(MSD_OK != chose_one_avail_fd(back_end, worker))
        {
            MSD_ERROR_LOG("Chose_one_avail_fd failed!");
            //goto deal_one_err;
        }
    }
    else
    {
        goto deal_one_err;
    }

    return back_end;
    
deal_one_err:
    msd_str_free(back_end->recvbuf);
    msd_str_free(back_end->sendbuf);
    msd_dlist_destroy(back_end->address_list);
    free(back_end);
    return NULL;
}

int chose_one_avail_fd(back_end_t *back_end, msd_thread_worker_t *worker)
{
    assert(back_end);
    
    char err_buf[1024];
    msd_dlist_iter_t   dlist_iter;
    msd_dlist_node_t  *node;
    tos_addr_t        *addr;
    int address_len = back_end->address_list->len;
    int fd_arr[address_len];
    int i,n;
    fd_set wset;  
    struct timeval tval;  
    int error,code;
    socklen_t len;
    int find_flag = 0;
    int ret;
    char buf[2];
    
    for(i=0; i<address_len; i++)
    {
        fd_arr[i] = -1;
    }
    
    if(back_end->fd == -1 || back_end->status == B_BAD)
    {
again:    
        msd_dlist_rewind(back_end->address_list, &dlist_iter);
        i = 0;
        FD_ZERO(&wset);  
        tval.tv_sec  = 0;  
        tval.tv_usec = 50000; //50毫秒
        while ((node = msd_dlist_next(&dlist_iter))) 
        {
            addr = node->value;
            memset(err_buf, 0, 1024);
            fd_arr[i] = msd_anet_tcp_nonblock_connect(err_buf, addr->back_ip, addr->back_port);
            FD_SET(fd_arr[i], &wset);           
            //printf("%s    %d      fd:%d\n", addr->back_ip, addr->back_port, fd_arr[i]);
            i++;
        }
        
        if ((n = select(fd_arr[i-1]+1, NULL, &wset, NULL, &tval)) == 0) 
        {  
            for(i=0; i<address_len; i++)
            {
                close(fd_arr[i]);
            }
            MSD_ERROR_LOG("Select time out, No fd ok!");
            return MSD_ERR; 
        }

        for(i=0; i<address_len; i++)
        {
            error = 0;
            if (FD_ISSET(fd_arr[i], &wset)) 
            {  
                len = sizeof(error);  
                code = getsockopt(fd_arr[i], SOL_SOCKET, SO_ERROR, &error, &len);  
                /* 如果发生错误，Solaris实现的getsockopt返回-1，把pending error设置给errno. Berkeley实现的 
                 * getsockopt返回0, pending error返回给error. 需要处理这两种情况 */  
                if (code < 0 || error) 
                {  
                    MSD_WARNING_LOG("Fd:%d not avail!", fd_arr[i]);
                    if (error)   
                        errno = error; 
                    continue;
 
                }
                else
                {
                    /* 发现可用连接 */
                    find_flag        = 1;
                    back_end->idx    = i;
                    back_end->fd     = fd_arr[i];
                    back_end->status = B_OK;
                    node = msd_dlist_index(back_end->address_list, i);
                    addr = node->value; 
                    MSD_INFO_LOG("Find the ok fd:%d, IP:%s, Port:%d",fd_arr[i], addr->back_ip, addr->back_port);
                    //printf("Find the ok fd:%d\n",fd_arr[i]);
                    break;
                }
            }       
        }
        
        if(!find_flag)
        {
            MSD_ERROR_LOG("No fd ok!");
            for(i=0; i<address_len; i++)
            {
                back_end->idx = -1;
                close(fd_arr[i]);
            }
            return MSD_ERR;             
        }
        else
        {
            //关闭没有选中的全部fd
            for(i=0; i<address_len; i++)
            {
                if(back_end->fd != fd_arr[i]) 
                {
                    MSD_DEBUG_LOG("Close the not chosen fd:%d", fd_arr[i]);
                    close(fd_arr[i]);
                }
            }    
            
            //注册读取函数
            if (msd_ae_create_file_event(worker->t_ael, back_end->fd, MSD_AE_READABLE,
                            read_from_back, worker) == MSD_ERR) 
            {
                MSD_ERROR_LOG("Create read file event failed. IP:%s, Port:%d",
                        addr->back_ip, addr->back_port);
                return MSD_ERR;
            }
        }
    }
    else
    {
        //探测现有fd可用性
        memset(buf, 0, 2);
        ret = recv(back_end->fd, buf, 1, MSG_PEEK);
        if(ret == 0)
        {
            //对方关闭连接，启动挑选流程
            node = msd_dlist_index(back_end->address_list, back_end->idx);
            addr = node->value; 
            MSD_WARNING_LOG("Peer close. Current fd:%d not ok, IP:%s, Port:%d, chose again!", back_end->fd, addr->back_ip, addr->back_port);
            msd_ae_delete_file_event(worker->t_ael, back_end->fd, MSD_AE_WRITABLE | MSD_AE_READABLE);
            close(back_end->fd);
            back_end->idx    = -1;
            back_end->fd     = -1;
            back_end->status = B_BAD;
            goto again;
        }
        else if(ret < 0)
        {
            if (errno == EAGAIN || errno==EINTR)
            {
                MSD_DEBUG_LOG("Normal:%s. Current fd:%d ok!",strerror(errno), back_end->fd);
            }
            else
            {
                //未知错误，启动挑选流程
                node = msd_dlist_index(back_end->address_list, back_end->idx);
                addr = node->value; 
                MSD_WARNING_LOG("Unkonw error:%s. Current fd:%d not ok, IP:%s, Port:%d, chose again!",strerror(errno), back_end->fd, addr->back_ip, addr->back_port);
                msd_ae_delete_file_event(worker->t_ael, back_end->fd, MSD_AE_WRITABLE | MSD_AE_READABLE);
                close(back_end->fd);
                back_end->idx    = -1;
                back_end->fd     = -1;
                back_end->status = B_BAD;
                goto again;
            }
        }
        
        //OK! do nothing.
    }
    return MSD_OK;
}

void send_to_back(msd_ae_event_loop *el, int fd, void *privdata, int mask) 
{
    back_end_t *back_end = (back_end_t *)privdata;
    int nwrite;
    MSD_AE_NOTUSED(mask);

    msd_dlist_node_t *node = msd_dlist_index(back_end->address_list, back_end->idx);
    tos_addr_t       *addr = node->value; 
                
    nwrite = write(fd, back_end->sendbuf->buf, back_end->sendbuf->len);
    back_end->access_time = time(NULL);
    if (nwrite < 0) 
    {
        if (errno == EAGAIN) /* 非阻塞fd写入，可能暂不可用 */
        {
            MSD_WARNING_LOG("Write to back temporarily unavailable! fd:%d. IP:%s, Port:%d", back_end->fd, addr->back_ip, addr->back_port);
            nwrite = 0;
        } 
        else if(errno==EINTR)/* 遭遇中断 */
        {  
            MSD_WARNING_LOG("Write to back was interupted! fd:%d. IP:%s, Port:%d", back_end->fd, addr->back_ip, addr->back_port);
            nwrite = 0; 
        } 
        else 
        {
            MSD_ERROR_LOG("Write to back failed! fd:%d. IP:%s, Port:%d. Error:%s", back_end->fd, addr->back_ip, addr->back_port, strerror(errno));
            return;
        }
    }
    MSD_INFO_LOG("Write to back! fd:%d. IP:%s, Port:%d. content:%s", back_end->fd, addr->back_ip, addr->back_port, back_end->sendbuf->buf);

    /* 将已经写完的数据，从sendbuf中裁剪掉 */
    if(MSD_OK != msd_str_range(back_end->sendbuf, nwrite, back_end->sendbuf->len-1))
    {
        /* 如果已经写完了,则write_len == back_end->sendbuf->len。则msd_str_range返回NONEED */
        msd_str_clear(back_end->sendbuf);
    }

     /* 水平触发，如果sendbuf已经为空，则删除写事件，否则会不断触发
       * 注册写事件有so中的handle_process去完成 */
    if(back_end->sendbuf->len == 0)
    {
        msd_ae_delete_file_event(el, back_end->fd, MSD_AE_WRITABLE);
    }
    return;
}

void read_from_back(msd_ae_event_loop *el, int fd, void *privdata, int mask) 
{
    msd_thread_worker_t *worker = (msd_thread_worker_t *)privdata;
    back_end_t *back_end;
    tos_addr_t *addr;
    int nread;
    char buf[1024];
    MSD_AE_NOTUSED(el);
    MSD_AE_NOTUSED(mask);
    trap_worker_data_t *worker_data = worker->priv_data;;
    msd_dlist_t *dlist = worker_data->back_end_list;
    msd_dlist_node_t *node;
    msd_dlist_iter_t dlist_iter;
    msd_dlist_rewind(dlist, &dlist_iter);
    while ((node = msd_dlist_next(&dlist_iter))) 
    {
        back_end = node->value;  
        if(back_end->fd == fd)
            break;
    }

    node = msd_dlist_index(back_end->address_list, back_end->idx);
    addr = node->value;     
    
    nread = read(fd, buf, 1024 - 1);
    back_end->access_time = time(NULL);
    if (nread == -1) 
    {
        /* 非阻塞的fd，读取不会阻塞，如果无内容可读，则errno==EAGAIN */
        if (errno == EAGAIN) 
        {
            MSD_WARNING_LOG("Read back. IP:%s, Port:%d. eagain: %s",
                     addr->back_ip, addr->back_port, strerror(errno));
            nread = 0;
            return;
        }
        else if (errno == EINTR) 
        {
            MSD_WARNING_LOG("Read back. IP:%s, Port:%d. interrupt: %s",
                     addr->back_ip, addr->back_port, strerror(errno));
            nread = 0;
            return;
        } 
        else 
        {
            MSD_ERROR_LOG("Read back. IP:%s, Port:%d.failed: %s",
                     addr->back_ip, addr->back_port, strerror(errno));

            if(MSD_OK != chose_one_avail_fd(back_end, worker))
            {
                close(back_end->fd);
                MSD_DEBUG_LOG("Chose_one_avail_fd failed!");
            }
            return;
        }
    } 
    else if (nread == 0) 
    {
        MSD_WARNING_LOG("Back_end close. Fd:%d, IP:%s, Port:%d",back_end->fd, addr->back_ip, addr->back_port);
        msd_ae_delete_file_event(worker->t_ael, back_end->fd, MSD_AE_WRITABLE | MSD_AE_READABLE);
        close(back_end->fd);
        back_end->idx    = -1;
        back_end->fd     = -1;
        back_end->status = B_BAD;

        /* 如果后端关闭连接，则清空sendbuf和recvbuf，防止堆积的数据的开头，格式是错误的，会导致后面永远解析错误 */
        msd_str_clear(back_end->recvbuf);
        msd_str_clear(back_end->sendbuf);
        if(MSD_OK != chose_one_avail_fd(back_end, worker))
        {
            close(back_end->fd);
            MSD_DEBUG_LOG("Chose_one_avail_fd failed!");
        }        
        return;
    }
    buf[nread] = '\0';

    MSD_INFO_LOG("Read from back. Fd:%d, IP:%s, Port:%d. Length:%d, Content:%s", 
                    back_end->fd, addr->back_ip, addr->back_port, nread, buf);
    return;
}
