/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_ae.h
 * 
 * Description :  A simple event-driven programming library. It's from Redis.
 *                The default multiplexing layer is select. If the system 
 *                support epoll, you can define the epoll macro.
 *
 *                #define MSD_EPOLL_MODE
 *
 *     Created :  Apr 7, 2012 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/
#ifndef __MSD_AE_H_INCLUDED__
#define __MSD_AE_H_INCLUDED__

/*
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
       
#define MSD_OK 0
#define MSD_ERR -1

#define MSD_EPOLL_MODE
//#define MSD_SELECT_MODE
*/

#ifdef MSD_EPOLL_MODE
#define MSD_AE_SETSIZE     (1024 * 10)     /* Max number of fd supported */
#else
#define MSD_AE_SETSIZE     1024            /* select 最多监听1024个fd */
#endif

#define MSD_AE_NONE         0
#define MSD_AE_READABLE     1
#define MSD_AE_WRITABLE     2

#define MSD_AE_FILE_EVENTS  1
#define MSD_AE_TIME_EVENTS  2
#define MSD_AE_ALL_EVENTS   (MSD_AE_FILE_EVENTS | MSD_AE_TIME_EVENTS)
#define MSD_AE_DONT_WAIT    4

#define MSD_AE_NOMORE       -1

/* Macros，不使用，啥也不干 */
#define MSD_AE_NOTUSED(V)   ((void)V)

struct msd_ae_event_loop;

/* Type and data structures */
typedef void msd_ae_file_proc(struct msd_ae_event_loop *el, int fd, 
        void *client_data, int mask);
typedef int msd_ae_time_proc(struct msd_ae_event_loop *el, long long id,
        void *client_data);
typedef void msd_ae_event_finalizer_proc(struct msd_ae_event_loop *el,
        void *client_data);
typedef void msd_ae_before_sleep_proc(struct msd_ae_event_loop *el);

/* File event structure */
typedef struct msd_ae_file_event {
    int                  mask; /* one of AE_(READABLE|WRITABLE) */
    msd_ae_file_proc    *r_file_proc;
    msd_ae_file_proc    *w_file_proc;
    void *client_data;
} msd_ae_file_event;

/* Time event structure */
typedef struct msd_ae_time_event {
    long long id;                                /* 时间事件注册id */
    long when_sec;                               /* 事件发生的绝对时间的秒数 */
    long when_ms;                                /* 事件发生的绝对时间的毫秒数 */
    msd_ae_time_proc    *time_proc;              /* 超时回调函数 */
    msd_ae_event_finalizer_proc *finalizer_proc; /* 时间事件的析构回调 */
    void *client_data;                           /* 超时回调函数/析构函数的参数 */
    struct msd_ae_time_event *next;
} msd_ae_time_event;

/* A fired event */
typedef struct msd_ae_fired_event {
    int fd;
    int mask;
} msd_ae_fired_event;

#ifdef MSD_EPOLL_MODE

typedef struct msd_ae_api_state 
{
    int epfd;
    struct epoll_event events[MSD_AE_SETSIZE];
} msd_ae_api_state;
#else

/* select最大fd只支持1024! */
typedef struct msd_ae_api_state 
{
    fd_set rfds, wfds;
    /* We need to have a copy of the fd sets as it's not safe to
       reuse FD sets after select(). */
    fd_set _rfds, _wfds;
} msd_ae_api_state;
#endif

/* State of an event base program */
typedef struct msd_ae_event_loop {
    int maxfd;                                  /* highest file descriptor currently registered */
    long long           time_event_next_id;     /* 当有新的时间事件注册时，应该赋予它的注册id */
    time_t              last_time;              /* Used to detect system clock skew */
    msd_ae_file_event   events[MSD_AE_SETSIZE]; /* 文件事件数组，fd作为索引，它和api_data是对应的 */
    msd_ae_fired_event  fired[MSD_AE_SETSIZE];  /* Fired events，这个不是用fd作为索引的，从0开始 */
    msd_ae_time_event   *time_event_head;
    int stop;
    msd_ae_api_state    *api_data;              /* fd为索引，与events一一对应，用于selec/epoll函数轮询 */
    msd_ae_before_sleep_proc    *before_sleep;
} msd_ae_event_loop;

/* Prototypes */
msd_ae_event_loop   *msd_ae_create_event_loop(void);
void msd_ae_free_event_loop(msd_ae_event_loop *el);
void msd_ae_stop(msd_ae_event_loop *el);
int msd_ae_create_file_event(msd_ae_event_loop *el, int fd, int mask,
        msd_ae_file_proc *proc, void *client_data);
void msd_ae_delete_file_event(msd_ae_event_loop *el, int fd, int mask);
int msd_ae_get_file_event(msd_ae_event_loop *el, int fd);
long long msd_ae_create_time_event(msd_ae_event_loop *el, long long milliseconds,
        msd_ae_time_proc *proc, void *client_data,
        msd_ae_event_finalizer_proc *finalizer_proc);
int msd_ae_delete_time_event(msd_ae_event_loop *el, long long id);
int msd_ae_process_events(msd_ae_event_loop *el, int flags);
int msd_ae_wait(int fd, int mask, long long milliseconds);
void msd_ae_main(msd_ae_event_loop *el);
char *msd_ae_get_api_name(void);
void msd_ae_set_before_sleep_proc(msd_ae_event_loop *el, 
        msd_ae_before_sleep_proc *before_sleep);

#endif /*__MSD_AE_H_INCLUDED__ */
