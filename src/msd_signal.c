/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_signal.h 
 * 
 * Description :  Msd_signal, Mossad信号处理相关的内容.
 *                信号处理分为两个部分:
 *                1.对于因为程序逻辑需要而产生的信号，由专门的信号处理线程同步进行处理
 *                2.对会导致程序运行终止的信号如SIGSEGV/SIGBUS等，由各个线程自己异步处理
 * 
 *     Created :  May 17, 2012 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/
#include "msd_core.h"

extern msd_instance_t *g_ins;

volatile msd_signal_t g_signals[] = 
{
    {SIGTERM, "SIGTERM"},
    {SIGQUIT, "SIGQUIT"},
    {SIGCHLD, "SIGCHLD"},
    {SIGPIPE, "SIGPIPE"},            
    {SIGINT,  "SIGINT"},            
    {SIGSEGV, "SIGSEGV"},
    {SIGBUS,  "SIGBUS"},
    {SIGFPE,  "SIGFPE"},
    {SIGILL,  "SIGILL"},
    {SIGHUP,  "SIGHUP"},
    {0, NULL}
};

static const char *msd_get_sig_name(int sig);
static void msd_sig_segv_handler(int sig, siginfo_t *info, void *secret);
static void* msd_signal_thread_cycle(void *arg);
static void msd_public_signal_handler(int signo, msd_thread_signal_t *sig_worker);

/**
 * 功能: 根据信号数值获取信号名称
 * 参数: @sig,信号数
 * 说明: 
 *       如果获取不到，则返回NULL
 * 返回:成功:信号名; 失败:NULL
 **/
static const char *msd_get_sig_name(int sig)
{
    volatile msd_signal_t *psig;
    for (psig = g_signals; psig->signo != 0; ++psig) 
    {
        if (psig->signo == sig) 
        {
            return psig->signame;
        }
    }

    return NULL;
}

/**
 * 功能: 私有信号初始化
 * 说明: 
 *       对于SIGSEGV/SIGBUS/SIGFPE/SIGILL这四个信号
 *       由各个线程自行捕获处理
 * 返回:成功:0; 失败:-x
 **/
int msd_init_private_signals() 
{
    struct sigaction sa;

    /* 设置信号处理程序使用的栈的地址，如果没有被设置过，则默认会使用正常的用户栈 
     * 注意: 
     *      无论是否设置额外栈，当收到SIGSEGV信号之后，能否打印出栈，是一个随机结果
     *      具体原因待查
     **/
    /*
    static char alt_stack[SIGSTKSZ];
    stack_t ss = {
        .ss_size = SIGSTKSZ,
        .ss_sp = alt_stack,
    };
    sigaltstack(&ss, NULL);
    */

    /* 安装信号
     * SA_SIGINFO  :信号处理函数，还有外带参数 
     * SA_RESETHAND:当调用信号处理函数完毕后，将信号的处理函数重置为缺省值SIG_DFL
     * SA_NODEFER  :一般情况下， 当信号处理函数运行时，内核将阻塞该给定信号。
     *              但是如果设置了 SA_NODEFER标记， 那么在该信号处理函数运行时，内核将不会阻塞该信号
     * SA_ONSTACK  :如果设置了该标志，并使用sigaltstack（）或sigstack（）声明了备用信号堆栈，信号将
     *              会传递给该堆栈中的调用进程，否则信号在当前堆栈上传递。
     * SA_RESTART  :是否重启被中断的系统调用
     **/
    sigemptyset(&sa.sa_mask);/* 信号处理中时，不阻塞任何其他信号 */
    //sa.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND | SA_SIGINFO | SA_RESTART;
    sa.sa_flags = SA_RESETHAND | SA_SIGINFO | SA_RESTART;
    sa.sa_sigaction = msd_sig_segv_handler;
    sigaction(SIGSEGV, &sa, NULL); /* 发生段错误的时候收到此信号 */
    sigaction(SIGBUS, &sa, NULL);  /* 总线错误，非法地址, 包括内存地址对齐(alignment)出错 */
    sigaction(SIGFPE, &sa, NULL);  /* 在发生致命的算术运算错误时发出,浮点异常 */
    sigaction(SIGILL, &sa, NULL);  /* 执行了非法指令. 通常是因为可执行文件本身出现错误, 或者试图执行 
                                    * 数据段. 堆栈溢出时也有可能产生这个信号
                                    */
    return MSD_OK;
}

/**
 * 功能: 私有信号处理函数
 * 参数: @sig,信号数
 *       @info,外带参数
 *       @secret?
 * 说明: 
 *       对于SIGSEGV/SIGBUS/SIGFPE/SIGILL这四个信号
 *       处理函数，首先打印堆栈信息，然后恢复成信号的
 *       默认操作之后，再发送并处理一次信号
 * 注意:
 *       1. 因为注册信号的时候，指定了SA_NODEFER参数，所以可以在
 *          信号处理函数中，再次接收到此信号，不会阻塞
 *       2. 对于私有信号的这种处理，是传统的"异步"方式
 * 返回:成功:0; 失败:-x
 **/
static void msd_sig_segv_handler(int sig, siginfo_t *info, void *secret) 
{
    void *trace[100];
    char **messages = NULL;
    int i, trace_size = 0;
    struct sigaction sa;
    const char *sig_name;
    if(!(sig_name = msd_get_sig_name(sig)))
    {
        MSD_FATAL_LOG("Thread[%lu] receive signal: %d", pthread_self(), sig);
    }
    else
    {
        MSD_FATAL_LOG("Thread[%lu] receive signal: %s", pthread_self(), sig_name);
    }
    /* Generate the stack trace. */
    trace_size = backtrace(trace, 100);
    messages = backtrace_symbols(trace, trace_size);
    MSD_FATAL_LOG("--- STACK TRACE BEGIN--- ");
    /* 日志中打印出程序的调用栈 */
    for (i = 0; i < trace_size; ++i) 
    {
        MSD_FATAL_LOG("%s", messages[i]);
    }
    MSD_FATAL_LOG("--- STACK TRACE END--- ");

    
    /* 信号函数劫持了默认操作，为了防止默认操作有什么重要效果被忽略
     * 所以需要重新注册一次默认操作，再发送一次信号
     */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND;
    sa.sa_handler = SIG_DFL;
    sigaction(sig, &sa, NULL);
    raise(sig); /* raise：用于向进程自身发送信号。成功返回0，失败返回-1。 */
}

/**
 * 功能: 公共信号初始化
 * 说明:   
 *   1. 主线程设置信号掩码，阻碍希望同步处理的信号(主线程的信号掩码会被其创建的线程继承)
 *   2. 主线程创建信号处理线程；信号处理线程将希望同步处理的信号集设为 sigwait（）的第一个参数
 * 返回:成功:0; 失败:-x
 **/
int msd_init_public_signals() 
{
    sigset_t       bset, oset;        
    pthread_t      ppid;   
    pthread_attr_t attr;
    char           error_buf[256];
    msd_thread_signal_t *sig_worker = calloc(1, sizeof(msd_thread_signal_t));
    if(!sig_worker)
    {
        MSD_ERROR_LOG("Create sig_worker failed");
        return MSD_ERR;
    }

    g_ins->sig_worker = sig_worker;
    
    /* 
     * 创建通信管道
     * signal读取出信号之后，将相应操作写给master
     **/
    int fds[2];
    if (pipe(fds) != 0)
    {
        MSD_ERROR_LOG("Thread signal creat pipe failed");
        return MSD_ERR;
    }
    sig_worker->notify_read_fd  = fds[0];
    sig_worker->notify_write_fd = fds[1];

    /* 管道不能是阻塞的! */
    if((MSD_OK != msd_anet_nonblock(error_buf,  sig_worker->notify_read_fd))
        || (MSD_OK != msd_anet_nonblock(error_buf,  sig_worker->notify_write_fd)))
    {
        close(sig_worker->notify_read_fd);
        close(sig_worker->notify_write_fd);
        MSD_ERROR_LOG("Set signal pipe nonblock failed, err:%s", error_buf);
        return MSD_ERR;        
    }

    /* 阻塞公共信号 */    
    sigemptyset(&bset);    
    sigaddset(&bset, SIGTERM);   
    sigaddset(&bset, SIGHUP); 
    sigaddset(&bset, SIGQUIT);
    sigaddset(&bset, SIGCHLD);
    sigaddset(&bset, SIGPIPE);
    sigaddset(&bset, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &bset, &oset) != 0)
    {
        MSD_ERROR_LOG("Set pthread mask failed");
        return MSD_ERR;
    }

    /* 创建专职信号处理线程 */  
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);    
    pthread_create(&ppid, NULL, msd_signal_thread_cycle, sig_worker);      
    pthread_attr_destroy(&attr);
    
    return MSD_OK;
}

/**
 * 功能: 专职信号处理线程
 * 说明: 
 *      1.将别的线程阻塞起来的信号，全部加入到自己的监听掩码中来
 *      2.无限循环，阻塞起来，等待自己监听的信号到来
 * 注意:
 *      1. 对于共有信号的这种处理，是"同步"方式
 * 返回:成功:0; 失败:-x
 **/
static void* msd_signal_thread_cycle(void *arg)
{    
    sigset_t   waitset;    
    siginfo_t  info;    
    int        rc;
    msd_thread_signal_t *sig_worker = (msd_thread_signal_t *)arg;
    
    MSD_INFO_LOG("Worker[Signal] begin to work");
    
    /* 将别的线程阻塞的信号，全部加入自己的监听范围 */
    sigemptyset(&waitset);
    sigaddset(&waitset, SIGTERM);    
    sigaddset(&waitset, SIGQUIT);
    sigaddset(&waitset, SIGCHLD);
    sigaddset(&waitset, SIGPIPE);
    sigaddset(&waitset, SIGINT);
    sigaddset(&waitset, SIGHUP);

    /* 无线循环阻塞，等待信号到来 */
    while (1)  
    {       
        rc = sigwaitinfo(&waitset, &info);        
        if (rc != MSD_ERR) 
        {   
            /* 同步处理信号 */
            msd_public_signal_handler(info.si_signo, sig_worker);
        } 
        else 
        {            
            MSD_ERROR_LOG("Sigwaitinfo() returned err: %d; %s", errno, strerror(errno));       
        }    

     }
     return (void *)NULL;
}

/**
  * 功能: 同步信号处理
  * 参数: @信号数值
  * 说明: 
  *      1.如果收到SIGPIPE/SIGCHLD信号，忽略
  *      2.如果收到SIGQUIT/SIGTERM/SIGINT则通知master和worker退出
  *      3.在传统的信号处理函数中，需要考虑系统调用是可重入的，但
  *        本函数是同步信号处理函数，不用考虑
  **/
static void msd_public_signal_handler(int signo, msd_thread_signal_t *sig_worker) 
{
    const char *sig_name;
    int to_stop = 0;
    if(!(sig_name = msd_get_sig_name(signo)))
    {
        MSD_INFO_LOG("Thread[%lu] receive signal: %d", pthread_self(), signo);
    }
    else
    {
        MSD_INFO_LOG("Thread[%lu] receive signal: %s", pthread_self(), sig_name);
    } 
    
    switch (signo) 
    {
        case SIGQUIT: 
        case SIGTERM:
        case SIGINT:
            /* prepare for shutting down */
            to_stop = 1;
            break;
        case SIGPIPE:
        case SIGCHLD:
        case SIGHUP:
            /* do noting */
            break;
    }

    if(to_stop)
    {
        /* 通知master关闭，不需要考虑write可重入的问题 */
        if(4 != write(sig_worker->notify_write_fd, "stop", 4))
        {
            if(errno == EINTR)
            {
                write(sig_worker->notify_write_fd, "stop", 4);
                MSD_INFO_LOG("Worker[signal] exit!");
                pthread_exit(0);
            }
            else
            {
                MSD_ERROR_LOG("Pipe write error, errno:%d", errno);
            }
        }
        else
        {
            /* 发送完成之后，signal线程完成使命，退出 */
            MSD_INFO_LOG("Worker[signal] exit!");
            pthread_exit(0);
        }
    }
}


#ifdef __MSD_SIGNAL_TEST_MAIN__

void msd_thread_sleep(int s) 
{
    struct timeval timeout;
    timeout.tv_sec  = s;
    timeout.tv_usec = 0;
    select( 0, NULL, NULL, NULL, &timeout);
}

void segment_fault()
{
    char *ptr = 0;
    printf("%c", *ptr);
}

void* phtread_SIGSEGV(void *arg)
{
    msd_thread_sleep(1);
    int a = 0;
    printf("a=%d", a);
    /* 制造段错误 */
    segment_fault();

    return (void *)NULL;
}


void sig_int_handler(int sig)
{
    const char *sig_name;
    if(!(sig_name = msd_get_sig_name(sig)))
    {
        MSD_FATAL_LOG("Thread[%lu] receive signal: %d", pthread_self(), sig);
    }
    else
    {
        MSD_FATAL_LOG("Thread[%lu] receive signal: %s", pthread_self(), sig_name);
    }    
}

int sig_int_action()
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);/* 信号处理中时，不阻塞任何其他信号 */

    /* 测试SA_RESETHAND功效，如果没有此标志，收到SIGINT多次，就调用sig_int_handler多次，否则只能调用一次 */ 
    sa.sa_flags = SA_RESTART;
    //sa.sa_flags = SA_RESETHAND | SA_RESTART;
    sa.sa_handler = sig_int_handler;
    sigaction(SIGINT, &sa, NULL); /* 发生段错误的时候收到此信号 */ 
    return 0;
}

void * phtread_SIGINT(void *arg)
{
    while(1)
    {
        printf("%lu\n", pthread_self());
        msd_thread_sleep(10);
    }
    return (void *)NULL;
}


int main(int argc, char *argv[]) 
{
    int ret = msd_init_private_signals();
    msd_log_init( "./", "signal.log", 4, 100000000, 1, 0);
    MSD_INFO_LOG("Init ret:%d", ret);
    
    /* 用例1，单线程段错误 */
    /*
    char *ptr = NULL;
    // *ptr = 'c';
    printf("%c", *ptr);      
    */

    /* 用例2，多线程段错误，是否能打印出栈，结果随机，原因暂时不明 */
    /*
    int i;
    char *ptr = NULL;
    pthread_t      ppid;    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for(i=0; i<10; i++)
    {
        if(pthread_create(&ppid, &attr, phtread_SIGSEGV, NULL) != 0)
        {
            MSD_ERROR_LOG("Thread creat failed");
            return MSD_ERR;        
        }        
        else
        {
            MSD_INFO_LOG("Thread creat %lu", ppid);
        }
    }
    
    pthread_attr_destroy(&attr);
    //主线程段错误，先休眠，让子线程发生段错误
    msd_thread_sleep(10);
    printf("%c", *ptr); 
    */

    /* 用例3，测试单线程收到SIGINT，顺便测试SA_RESETHAND效果 */
    /*
    sig_int_action();
    msd_thread_sleep(100);
    printf("After receive sigint1\n");
    msd_thread_sleep(100);
    printf("After receive sigint2\n");
    */
    
    /* 用例4，在不做限定的情况下，查看接收信号的线程是否是随机的 */
    /*
    //测试结果显示，貌似永远是主线程接收到信号，和资料介绍有区别
    //但是无法确定这是否是必然现象
    sig_int_action();
    pthread_t      ppid;    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int i=0;
    for(i=0; i<10; i++)
    {
        if(pthread_create(&ppid, &attr, phtread_SIGINT, NULL) != 0)
        {
            MSD_ERROR_LOG("Thread creat failed");
            return MSD_ERR;        
        }
        else
        {
            MSD_INFO_LOG("Thread creat %lu", ppid);
        }
    }
    pthread_attr_destroy(&attr);
    //休眠，查看到底哪个线程能够收到信号
    while(1)
        msd_thread_sleep(100);
    */

    /* 用例5，启动专用信号线程，其他线程阻塞信号 */
    msd_init_public_signals();
    
    pthread_t      ppid;    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int i=0;
    for(i=0; i<10; i++)
    {
        if(pthread_create(&ppid, &attr, phtread_SIGINT, NULL) != 0)
        {
            MSD_ERROR_LOG("Thread creat failed");
            return MSD_ERR;        
        }
        else
        {
            MSD_INFO_LOG("Thread creat %lu", ppid);
        }
    }
    pthread_attr_destroy(&attr);
    //休眠，查看到底哪个线程能够收到信号
    while(1)
    {
        msd_thread_sleep(10);
        printf("%lu\n", pthread_self());
        /* 测试共有信号和私有信号都存在的情况 */
        //char *ptr = NULL;
        //*ptr = 'c';
    }
    return 0;
    
}

#endif /* __MSD_SIGNAL_TEST_MAIN__ */
