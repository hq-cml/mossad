/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_ae.c
 * 
 * Description :  A simple event-driven programming library. It's from Redis.
 *                The default multiplexing layer is select. If the system 
 *                support epoll, you can define the epoll macro.
 *
 *                #define MSD_EPOLL_MODE
 *
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/
#include "msd_core.h"

/**
 * 功能: 生成ae_loop->api_data的结构，并初始化之
 * 参数: @el, ae_loop指针
 * 注意:
 *      1. epoll不支持文件的描述符监听，只能用于网络，如果需要监听文件
 *         ，需要使用select。经测试，如果epoll监听文件，会报错EPERM
 *         (perror:Operation not permitted)
 * 返回: 成功，0 失败，NULL
 **/
#ifdef MSD_EPOLL_MODE

static int msd_ae_api_create(msd_ae_event_loop *el) 
{
    msd_ae_api_state *state = malloc(sizeof(*state));
    if (!state) 
    {
        return MSD_ERR;
    }

    state->epfd = epoll_create(MSD_AE_SETSIZE);
    if (state->epfd == -1) 
    {
        return MSD_ERR;
    }
    el->api_data = state;
    return 0;
}
#else

static int msd_ae_api_create(msd_ae_event_loop *el) 
{
    msd_ae_api_state *state = malloc(sizeof(*state));
    if (!state) 
    {
        return -1;
    }
    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    el->api_data = state;
    return MSD_OK;
}
#endif

/**
 * 功能: 生成ae_loop结构体，并初始化之
 * 返回: 成功，ae_lopp结构体指针, 失败，NULL
 **/
msd_ae_event_loop *msd_ae_create_event_loop(void) 
{
    msd_ae_event_loop *el;
    int i;

    el = (msd_ae_event_loop*)malloc(sizeof(*el));
    if (!el) 
        return NULL;
    
    el->last_time       = time(NULL);
    el->time_event_head = NULL;
    el->time_event_next_id = 0;
    el->stop = 0;
    el->maxfd = -1;
    el->before_sleep = NULL;
    if (msd_ae_api_create(el) != MSD_OK) 
    {
        free(el);
        return NULL;
    }

    /* Events with mask == MSD_AE_NONE are not set. 
       So let's initialize the vector with it. */
    for (i = 0; i < MSD_AE_SETSIZE; ++i) 
    {
        el->events[i].mask = MSD_AE_NONE;
    }
    return el;
}

/**
 * 功能: 析构api_data
 * 参数: @el
 **/
#ifdef MSD_EPOLL_MODE

static void msd_ae_api_free(msd_ae_event_loop *el) 
{
    msd_ae_api_state *state = el->api_data;
    close(state->epfd);
    free(state);
}
#else

static void msd_ae_api_free(msd_ae_event_loop *el) 
{
    if (el && el->api_data) 
    {
        free(el->api_data);
    }
}
#endif

/**
 * 功能: 析构ae_loop结构
 * 参数: @el
 **/
void msd_ae_free_event_loop(msd_ae_event_loop *el) 
{
    msd_ae_time_event *te, *next;
    /* 析构api_data */
    msd_ae_api_free(el);

    /* Delete all time event to avoid memory leak. */
    te = el->time_event_head;
    while (te) 
    {
        next = te->next;
        if (te->finalizer_proc) 
        {
            te->finalizer_proc(el, te->client_data);
        }
        free(te);
        te = next;
    }

    free(el);
}

/**
 * 功能: ae_loop轮训暂停
 **/
void msd_ae_stop(msd_ae_event_loop *el) 
{
    el->stop = 1;
}

/**
 * 功能: add event
 * 返回: 成功，0 失败，-x
 **/
#ifdef MSD_EPOLL_MODE

static int msd_ae_api_add_event(msd_ae_event_loop *el, int fd, int mask) 
{
    msd_ae_api_state *state = el->api_data;
    struct epoll_event ee;
    /* If the fd was already monitored for some event, we need a MOD
       operation. Otherwise we need an ADD operation. */
    int op = el->events[fd].mask == MSD_AE_NONE ? 
        EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    ee.events = 0;
    mask |= el->events[fd].mask; /* Merge old events. */
    /* 水平触发 */
    if (mask & MSD_AE_READABLE)
    {
        ee.events |= EPOLLIN;
    }

    if (mask & MSD_AE_WRITABLE) 
    {
        ee.events |= EPOLLOUT;
    }
    ee.data.u64 = 0;
    ee.data.fd = fd;
    if (epoll_ctl(state->epfd, op, fd, &ee) == -1) 
    {
        return MSD_ERR;
    }
    return MSD_OK;
}
#else

static int msd_ae_api_add_event(msd_ae_event_loop *el, int fd, int mask) 
{
    msd_ae_api_state *state = el->api_data;

    if (mask & MSD_AE_READABLE) 
    {
        FD_SET(fd, &state->rfds);
    }

    if (mask & MSD_AE_WRITABLE) 
    {
        FD_SET(fd, &state->wfds);
    }
    return MSD_OK;
}
#endif

/**
 * 功能: Register a file event. 
 * 参数: @el, ae_loop结构指针
 *       @fd, 文件或描述符fd
 *       @mask, 读或写标记
 *       @proc, 回调函数
 *       @client_data, 回调函数参数
 * 描述:
 *      1. fd会作为el->events[]的索引
 * 返回: 成功，0 失败，-x
 **/
int msd_ae_create_file_event(msd_ae_event_loop *el, int fd, int mask,
        msd_ae_file_proc *proc, void *client_data) 
{
    if (fd >= MSD_AE_SETSIZE) 
    {
        MSD_ERROR_LOG("fd:%d, MSD_AE_SETSIZE:%d", fd, MSD_AE_SETSIZE);
        return MSD_ERR;
    } 
    msd_ae_file_event *fe = &el->events[fd];

    if (msd_ae_api_add_event(el, fd, mask) == -1) 
    {
        return MSD_ERR;
    }

    fe->mask |= mask; /* On one fd, multi events can be registered. */
    if (mask & MSD_AE_READABLE) 
    {
        fe->r_file_proc = proc;
        fe->read_client_data = client_data;
    }

    if (mask & MSD_AE_WRITABLE) 
    {
        fe->w_file_proc = proc;
        fe->write_client_data = client_data;
    } 
    /* Ae bug. 读写事件用同一个参数，不合理 */
    //fe->client_data = client_data;
    
    /* Once one file event has been registered, the el->maxfd
       no longer is -1. */
    if (fd > el->maxfd) 
    {
        el->maxfd = fd;
    }
    return MSD_OK;
}

/**
 * 功能: delete event
 * 返回: 成功，0失败，-x
 **/
#ifdef MSD_EPOLL_MODE

static void msd_ae_api_del_event(msd_ae_event_loop *el, int fd, int delmask) 
{
    msd_ae_api_state *state = el->api_data;
    struct epoll_event ee;
    int mask = el->events[fd].mask & (~delmask);
    ee.events = 0;
    if (mask & MSD_AE_READABLE) 
    {
        ee.events |= EPOLLIN;
    }

    if (mask & MSD_AE_WRITABLE) 
    {
        ee.events |= EPOLLOUT;
    }
    ee.data.u64 = 0;
    ee.data.fd = fd;
    if (mask != MSD_AE_NONE) 
    {
        epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee);
    } 
    else 
    {
        /* Note, kernel < 2.6.9 requires a non null event pointer even
           for EPOLL_CTL_DEL. */
        epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, &ee);
    }
} 
#else

static void msd_ae_api_del_event(msd_ae_event_loop *el, int fd, int mask) 
{
    msd_ae_api_state *state = el->api_data;
    if (mask & MSD_AE_READABLE) 
    {
        FD_CLR(fd, &state->rfds);
    }

    if (mask & MSD_AE_WRITABLE) 
    {
        FD_CLR(fd, &state->wfds);
    }
}
#endif

/**
 * 功能: Unregister a file event
 * 参数: el, fd, mask 
 **/
void msd_ae_delete_file_event(msd_ae_event_loop *el, int fd, int mask) 
{
    if (fd >= MSD_AE_SETSIZE) 
    {
        return;
    }

    msd_ae_file_event *fe = &el->events[fd];
    if (fe->mask == MSD_AE_NONE) 
    {
        return;
    }
    fe->mask = fe->mask & (~mask);
    if (fd == el->maxfd && fe->mask == MSD_AE_NONE) 
    {
        /* All the events on the fd were deleted, update the max fd. */
        int j;
        for (j = el->maxfd - 1; j >= 0; --j) 
        {
            if (el->events[j].mask != MSD_AE_NONE) 
            {
                break;
            }
        }
        /* If all the file events on all fds deleted, max fd will get
           back to -1. */
        el->maxfd = j;
    }
    msd_ae_api_del_event(el, fd, mask);
}

/**
 * 功能: Get the file event by fd
 * 返回: 成功，MSD_AE_READABLE/MSD_AE_WRITEABLE.失败，0
 **/
int msd_ae_get_file_events(msd_ae_event_loop *el, int fd) 
{
    if (fd >= MSD_AE_SETSIZE) 
    {
        return 0;
    }

    msd_ae_file_event *fe = &el->events[fd];
    return fe->mask;
}

/**
 * 功能: get the now time
 * 参数: @seconds, @milliseconds
 * 描述:
 *      1. 当前时间的秒和毫秒数将被赋值给参数second, milliseconds
 **/
static void msd_ae_get_now_time(long *seconds, long *milliseconds) 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *seconds = tv.tv_sec;              /* 秒 */
    *milliseconds = tv.tv_usec / 1000; /* 微秒=>毫秒 */
}

/**
 * 功能: 获取当前时间之后的milliseconds毫秒后的时间
 * 参数: @milliseconds, @sec, @ms
 * 描述:
 *      1. 未来时间的秒和毫秒数将被赋值给参数sec, ms
 **/
static void msd_ae_add_milliseconds_to_now(long long milliseconds, 
        long *sec, long *ms) 
{
    long cur_sec, cur_ms, when_sec, when_ms;
    msd_ae_get_now_time(&cur_sec, &cur_ms);
    when_sec = cur_sec + milliseconds / 1000;
    when_ms = cur_ms + milliseconds % 1000;
    /* cur_ms < 1000, when_ms < 2000, so just one time is enough. */
    if (when_ms >= 1000) 
    {
        ++when_sec;
        when_ms -= 1000;
    }
    *sec = when_sec;
    *ms = when_ms;
}

/**
 * 功能: Register a time event. 
 * 参数: @ms：发生时间，当前时间的ms毫秒之后
 *       @proc：时间的回调
 *       @client_data：回调函数的参数
 *       @finalizer_proc：时间事件的析构回调
 * 描述:
 *      1. time_event_next_id从0开始自增，第一个时间事件id=0
 *      2. 时间事件是一个链表结构，每个节点是一个事件
 *      3. 每新增一个时间事件节点，插入到链表头部
 * 返回: 成功，时间事件的id号。失败，-x
 **/
long long msd_ae_create_time_event(msd_ae_event_loop *el, long long ms, 
        msd_ae_time_proc *proc, void *client_data,
        msd_ae_event_finalizer_proc *finalizer_proc) 
{
    long long id = el->time_event_next_id++;
    msd_ae_time_event *te;
    te = (msd_ae_time_event*)malloc(sizeof(*te));
    if (te == NULL) 
    {
        return MSD_ERR;
    }
    te->id = id;
    msd_ae_add_milliseconds_to_now(ms, &te->when_sec, &te->when_ms);
    te->time_proc = proc;
    te->finalizer_proc = finalizer_proc;
    te->client_data = client_data;
    
    /* Insert time event into the head of the linked list. */
    te->next = el->time_event_head;
    el->time_event_head = te;
    return id;
}

/**
 * 功能: Unregister a time event.
 * 参数: @el, @id:时间事件id 
 * 描述:
 *      1. el->time_event_next_id不需要有相关的操作，因为id只是一个号
 * 返回: 成功，0 失败，-x
 **/
int msd_ae_delete_time_event(msd_ae_event_loop *el, long long id) 
{
    msd_ae_time_event *te, *prev = NULL;

    te = el->time_event_head;
    while (te) 
    {
        if (te->id == id) 
        {
            /* Delete a node from the time events linked list. */
            if (prev == NULL) 
            {
                el->time_event_head = te->next;
            } 
            else 
            {
                prev->next = te->next;
            }
            if (te->finalizer_proc) 
            {
                /* 预防内存泄露 */
                te->finalizer_proc(el, te->client_data);
            }
            free(te);
            return MSD_OK;
        }
        prev = te;
        te = te->next;
    }
    return MSD_ERR; /* NO event with the specified ID found */
}

/**
 * 功能: 遍历时间事件链表，找到最先的应该触发的时间事件。
 * 参数: @el
 * 描述:
 *      1. This operation is useful to know how many time the select can be
 *         put in sleep without to delay any event. If there are no timers 
 *         NULL is returned.
 *      2. Note that's O(N) since time events are unsorted. Possible optimizations
 *         (not needed by Redis so far, but ...):
 *         1) Insert the event in order, so that the nearest is just the head.
 *            Much better but still insertion or deletion of timer is O(N).
 *         2) Use a skiplist to have this operation as O(1) and insertion as
 *            O(log(N)).
 * 返回: 成功，时间事件节点指针 失败，
 **/
static msd_ae_time_event *msd_ae_search_nearest_timer(msd_ae_event_loop *el) 
{
    msd_ae_time_event *te = el->time_event_head;
    msd_ae_time_event *nearest = NULL;

    while (te) 
    {
        if (!nearest || te->when_sec < nearest->when_sec ||
                (te->when_sec == nearest->when_sec &&
                 te->when_ms < nearest->when_ms)) 
        {
            nearest = te;
        }
        te = te->next;
    }
    return nearest;
}

/**
 * 功能: Process time events.
 * 参数: @el
 * 描述:
 *      1. time_proc函数不能含有阻塞操作，否则会影响整个时间队列处理
 *      2. time_proc函数的返回值是相对于当前时间点，下一次触发自己的毫秒数
 *      3. time_proc函数中可能会注册新的时间事件，为了防止msd_process_time_events
 *         无限循环执行下去，记录maxid，本轮不处理新注册的时间事件
 *      4. time_proc如果返回0，说明需要撤销此时间事件，这样就需要删除该节点，
 *         所以链表结构就发生了变化，所以每次处理完都从头再次遍历链表
 *      5. 新版redis增加了一个last_time的字段，作用是防止系统时间出现问题，如果
 *         程序初始时系统时间提前，后又被调回，则强迫所有时间事件先立刻触发一次
 * 返回: 成功，本轮处理时间个数 失败，
 **/
static int msd_process_time_events(msd_ae_event_loop *el) 
{
    int processed = 0;
    msd_ae_time_event *te;
    long long maxid;
    time_t now = time(NULL);

    /* If the system clock is moved to the future, and then set back to the
     * right value, time events may be delayed in a random way. Often this
     * means that scheduled operations will not be performed soon enough.
     *
     * Here we try to detect system clock skews, and force all the time
     * events to be processed ASAP when this happens: the idea is that
     * processing events earlier is less dangerous than delaying them
     * indefinitely, and practice suggests it is. */
    if (now < el->last_time) {
        te = el->time_event_head;
        while(te) {
            te->when_sec = 0;
            te = te->next;
        }
    }
    el->last_time = now;
    
    te = el->time_event_head;
    maxid = el->time_event_next_id - 1;

    /* 循环遍历时间事件链表 */
    while (te) 
    {
        long now_sec, now_ms;
        long long id;
        /* Don't process the time event registered during this process. */
        if (te->id > maxid) 
        { 
            te = te->next;
            continue;
        }
        msd_ae_get_now_time(&now_sec, &now_ms);
        /* timeout */ 
        if (now_sec > te->when_sec ||
            (now_sec == te->when_sec && now_ms >= te->when_ms)) 
        {
            /* 满足超时条件，则触发超时回调 */
            int ret;
            id = te->id;
            ret = te->time_proc(el, id, te->client_data);
            processed++;
            /* After an event is processed our time event list may no longer be the same, 
             * so we restart from head. Still we make sure to don't process events 
             * registered by event handlers itself in order to don't loop forever.
             * To do so we saved the max ID we want to handle.
             *
             * FUTURE OPTIMIZATIONS:
             * Note that this is NOT great algorithmically. Redis uses a single time event
             * so it's not a problem but the right way to do this is to add the new elements
             * on head, and to flag deleted elements in a special way for later deletion 
             * (putting references to the nodes to delete into another linked list). 
             */
            if (ret > 0) 
            {
                /* 没处理完，time_porc应该返回下次触发时间 */
                msd_ae_add_milliseconds_to_now(ret, &te->when_sec, &te->when_ms);
            } 
            else 
            {
                /* 处理完了，则直接删除节点 */
                msd_ae_delete_time_event(el, id);
            }

            te = el->time_event_head;
        } 
        else 
        {
            te = te->next;
        }
    }
    return processed;
}

/**
 * 功能: poll函数
 * 参数: @el，@tvp，poll的超时时间指针
 * 描述:
 *      1. 
 * 返回: 成功，触发的file事件个数。 失败，
 **/
#ifdef MSD_EPOLL_MODE

static int msd_ae_api_poll(msd_ae_event_loop *el, struct timeval *tvp) 
{
    msd_ae_api_state *state = el->api_data;
    int retval, numevents = 0;
    retval = epoll_wait(state->epfd, state->events, MSD_AE_SETSIZE,
        tvp ? (tvp->tv_sec * 1000 + tvp->tv_usec / 1000) : -1);
    if (retval > 0) 
    {
        int j;
        numevents = retval;
        for (j = 0; j < numevents; ++j) 
        {
            int mask = 0;
            struct epoll_event *e = state->events + j;
            if (e->events & EPOLLIN) 
            {
                mask |= MSD_AE_READABLE;
            }

            if (e->events & EPOLLOUT) 
            {
                mask |= MSD_AE_WRITABLE;
            }
            el->fired[j].fd = e->data.fd;
            el->fired[j].mask = mask;
        }
    }
    return numevents;
}
#else

static int msd_ae_api_poll(msd_ae_event_loop *el, struct timeval *tvp) 
{
    msd_ae_api_state *state = el->api_data;
    int ret, j, numevents = 0;
    /* 备份 */
    memcpy(&state->_rfds, &state->rfds, sizeof(fd_set));
    memcpy(&state->_wfds, &state->wfds, sizeof(fd_set));

    ret = select(el->maxfd + 1, &state->_rfds, &state->_wfds, NULL, tvp);
    if (ret > 0) 
    {
        /* 如果在超时时间tvp内有file事件触发，则应该写入fired数组中 */
        for (j = 0; j <= el->maxfd; ++j) 
        {
            int mask = 0;
            msd_ae_file_event *fe = &el->events[j];
            if (fe->mask == MSD_AE_NONE) 
            {
                continue;
            }
            if (fe->mask & MSD_AE_READABLE && FD_ISSET(j, &state->_rfds)) 
            {
                mask |= MSD_AE_READABLE;
            }
            if (fe->mask & MSD_AE_WRITABLE && FD_ISSET(j, &state->_wfds)) 
            {
                mask |= MSD_AE_WRITABLE;
            }

            /* 将触发的file事件入fired数组，通过mask来判断触发了读还是写 */
            el->fired[numevents].fd = j;
            el->fired[numevents].mask = mask;
            numevents++;
        }
    }
    return numevents;
}

#endif


/**
 * 功能: Process every pending event
 * 参数: @el, @flag
 * 描述:
 *      1. Process every pending time event, then every pending file event
 *         (that may be registered by time event callbacks just processed).
 *         Without special flags the function sleeps until some file event
 *         fires, or when the next time event occurs (if any).
 *
 *          If flags is 0, the function does nothing and returns.
 *          if flags has AE_ALL_EVENTS set, all the kind of events are processed.
 *          if flags has AE_FILE_EVENTS set, file events are processed.
 *          if flags has AE_TIME_EVENTS set, time events are processed.
 *          if flags has AE_DONT_WAIT set the function returns ASAP until all
 *          the events that's possible to process without to wait are processed.
 *      2. 如果没有时间事件，并且默认AE_DONT_WAIT未设置，则poll会永远阻塞，直到file
 *         事件触发，所以应该尽量至少设置一个时间事件
 * 注意：
 *      1. 对于所有类型的事件处理函数，都应该避免出现阻塞，否则会拖累整个时间
 *         链表上的事件触发处理!!!!!!!（见测试用例）
 * 返回: 成功，the number of events processed. 失败，
 **/
int msd_ae_process_events(msd_ae_event_loop *el, int flags) 
{
    int processed = 0, numevents;

    /* Nothing to do ? return ASAP */
    if (!(flags & MSD_AE_TIME_EVENTS) && !(flags & MSD_AE_FILE_EVENTS)) 
    {
        return 0;
    }

    /* Note that we want call select() even if there are no file
       events to process as long as we want to process time events,
       in order to sleep until the next time event is ready to fire. */
    if (el->maxfd != -1 ||
        ((flags & MSD_AE_TIME_EVENTS) && !(flags & MSD_AE_DONT_WAIT))) 
    {
        /* 有file事件被注册，或者(要等待时间时间且不是立即退出的) */
        int j;
        msd_ae_time_event *shortest = NULL;
        struct timeval tv, *tvp;
        
        if (flags & MSD_AE_TIME_EVENTS && !(flags & MSD_AE_DONT_WAIT)) 
        {
            shortest = msd_ae_search_nearest_timer(el);
        } 
        /* 
         *  基本原理:先找出一件最近就会发生的事情，然后算出离此刻的时间距离，在此距离内可以用select阻塞
         *           等过了这个时间距离，如果仍然没有file事件，select自动解开阻塞，去处理时间事件。
         *           这个参数tv就是这个时间距离tvp是指向tv的指针。
         */
        if (shortest) 
        {
            long now_sec, now_ms;

            /* Calculate the time missing for the nearest timer 
               to fire. store it in the tv */
            msd_ae_get_now_time(&now_sec, &now_ms);
            tvp = &tv;
            tvp->tv_sec = shortest->when_sec - now_sec;
            if (shortest->when_ms < now_ms) 
            {
                tvp->tv_usec = ((shortest->when_ms + 1000) - now_ms) * 1000;
                --tvp->tv_sec;
            } 
            else 
            {
                tvp->tv_usec = (shortest->when_ms - now_ms) * 1000;
            }

            if (tvp->tv_sec < 0) 
            {
                tvp->tv_sec = 0;
            }

            if (tvp->tv_usec < 0) 
            {
                tvp->tv_usec = 0;
            }
        } 
        else 
        {
            /* 如果没有时间事件，不推荐!! */ 
            /* 默认情况下MSD_AE_DONT_WAIT未设置，poll会一直阻塞直到file事件触发，
             * 若设置了立刻退出，则tv被置为0，select会立刻退出 */
            /* If we have to check for events but need to return
               ASAP because of AE_DONT_WAIT we need to set the 
               timeout to zero. */
            if (flags & MSD_AE_DONT_WAIT) 
            {
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            } 
            else 
            {
                /* Otherwise we can block. */
                tvp = NULL; /* wait forever，阻塞无超时 */
            }
        }

        /* block，调用select/epool处理file时间，tvp指示的时间，表示最多可以阻塞的时间 */
        numevents = msd_ae_api_poll(el, tvp);
        
        /* 处理完所有的file事件(在函数msd_ae_api_poll中，fired[]是不断覆盖的) */
        for (j = 0; j < numevents; ++j) 
        {
            msd_ae_file_event *fe = &el->events[el->fired[j].fd];
            
            int mask = el->fired[j].mask;
            int fd   = el->fired[j].fd;
            int rfired = 0;/* 是否触发了读事件 */
            
            /* Note the fe->mask & mask & ... code: maybe an already
               processed event removed an element that fired and we
               still didn't processed, so we check if the events is 
               still valid. */
            /* 
             * 翻译:events数组中的元素，可能被一个已经处理了的事件修改了，此时
             * fired事件还没处理呢，所以要先验证一下events数组对应元素状态
             */   
            if (fe->mask & mask & MSD_AE_READABLE) 
            {
                rfired = 1;
                fe->r_file_proc(el, fd, fe->read_client_data, mask);
            } 

            if (fe->mask & mask & MSD_AE_WRITABLE) 
            {
                /* 确保如果读和写是同一个函数的时候，不会重复执行 */
                if (!rfired || fe->w_file_proc != fe->r_file_proc) 
                {
                    fe->w_file_proc(el, fd, fe->write_client_data, mask);
                }
            }

            ++processed;
        }
    }
    /* Check time events */
    /* 处理所有的时间事件 */
    if (flags & MSD_AE_TIME_EVENTS) 
    {
        processed += msd_process_time_events(el);
    }

    /* Return the number of processed file/time events */
    return processed; 
}
/**
 * 功能: Wait for milliseconds until the given file descriptior becomes
 *       writable/readable/exception
 * 参数: @
 * 描述:
 *      1. 单独对某个文件的fd做select，等待milliseconds毫秒
 * 返回: 成功， 失败，
 **/
int msd_ae_wait(int fd, int mask, long long milliseconds) 
{
    struct timeval tv;
    fd_set rfds, wfds, efds;
    int retmask = 0, retval;

    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    if (mask & MSD_AE_READABLE) 
    {
        FD_SET(fd, &rfds);
    }
    
    if (mask & MSD_AE_WRITABLE) 
    {
        FD_SET(fd, &wfds);
    }
    
    if ((retval = select(fd + 1, &rfds, &wfds, &efds, &tv)) > 0) 
    {
        if (FD_ISSET(fd, &rfds)) 
        {
            retmask |= MSD_AE_READABLE;
        }
        if (FD_ISSET(fd, &wfds)) 
        {
            retmask |= MSD_AE_WRITABLE;
        }
        return retmask;
    } 
    else 
    {
        return retval;
    }
}

/**
 * 功能: main loop of the event-driven framework 
 * 参数: @el
 * 描述:
 *      1. 如果before_sleep非空，则先调用之，然后进入主体loop
 **/
void msd_ae_main_loop(msd_ae_event_loop *el) 
{
    el->stop = 0;
    while (!el->stop) 
    {
        if (el->before_sleep != NULL) 
        {
            el->before_sleep(el);
        }
        msd_ae_process_events(el, MSD_AE_ALL_EVENTS);
    }
}
 
/**
 * 功能: set before_sleep_proc
 * 参数: @el, @before_sleep
 */
void msd_ae_set_before_sleep_proc(msd_ae_event_loop *el, 
        msd_ae_before_sleep_proc *before_sleep) 
{
    el->before_sleep = before_sleep;
}

#ifdef __MSD_AE_TEST_MAIN__
int print_timeout(msd_ae_event_loop *el, long long id, void *client_data) 
{
    printf("Hello, AE!\n");
    return 5000; /* return AE_NOMORE */
}

struct items {
    char        *item_name;
    int         freq_sec;
    long long   event_id;
} acq_items [] = {
    {"acq1", 10 * 1000, 0},
    {"acq2", 30 * 1000, 0},
    {"acq3", 60 * 1000, 0},
    {"acq4", 10 * 1000, 0},
    {NULL, 0, 0}
};

int output_result(msd_ae_event_loop *el, long long id, void *client_data) 
{
    int i = 0;
    for ( ; ; ++i) 
    {
        if (!acq_items[i].item_name) 
        {
            break;
        }
        if (id == acq_items[i].event_id) 
        {
            printf("%s:%d\n", acq_items[i].item_name, (int)time(NULL));
            fflush(stdout);
            return acq_items[i].freq_sec;
        }
    }
    return 0;
}

/*添加时间事件*/
void add_all(msd_ae_event_loop *el) 
{
    int i = 0;
    for ( ; ; ++i) 
    {
        if (!acq_items[i].item_name) 
        {
            break;
        }
        acq_items[i].event_id = msd_ae_create_time_event(el, 
            acq_items[i].freq_sec,
            output_result, NULL, NULL);
    }
}

/*删除时间事件*/
void delete_all(msd_ae_event_loop *el) 
{
    int i = 0;
    for ( ; ; ++i) {
        if (!acq_items[i].item_name) {
            break;
        } 

        msd_ae_delete_time_event(el, acq_items[i].event_id);
    }
}

/* file事件 
 * 事件的处理应避免阻塞！！！！！
 */
void file_process(struct msd_ae_event_loop *el, int fd, void *client_data, int mask)
{
    char buf[1000] = {0};
    if(mask & MSD_AE_WRITABLE)
    {
        write( fd, client_data, strlen(client_data));
        /* 模拟阻塞的情况 */
        //sleep(1);
    }
    
    if (mask & MSD_AE_READABLE)
    {
        read(fd, buf, 1000);
        //printf("read:%s\n", buf);
        //sleep(2);/* 模拟阻塞的情况 */
    }
}


int main(int argc, char *argv[]) 
{
    //long long id;
    int mask, fd;

    fd = open("test_ae.file", O_RDWR );
    mask = MSD_AE_WRITABLE | MSD_AE_READABLE;
    msd_ae_event_loop *el = msd_ae_create_event_loop();
    add_all(el);
    /* 如果是epoll，不支持 */
    msd_ae_create_file_event(el, fd, mask, file_process, "hello\n");
    msd_ae_main_loop(el);
    delete_all(el);
    exit(0);
}
#endif /* __MSD_AE_TEST_MAIN__ */

