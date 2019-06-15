/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_signal.h 
 * 
 * Description :  Msd_signal, Mossad信号处理相关的内容.
 *                信号处理分为两个部分:
 *                1.对于因为程序逻辑需要而产生的信号，由专门的信号处理线程同步进行处理
 *                2.对会导致程序运行终止的信号如SIGSEGV/SIGBUS等，由各个线程自己处理
 * 
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/
#ifndef __MSD_SIGNAL_H_INCLUDED__
#define __MSD_SIGNAL_H_INCLUDED__

/*
#include <execinfo.h>
*/

typedef struct 
{
    int         signo;                 /* 信号数值 */
    char        *signame;              /* 信号宏名 */
} msd_signal_t;

typedef struct thread_signal{
    pthread_t          tid;             /* 线程id */
    int                notify_read_fd;  /* 和master线程通信管道读取端 */
    int                notify_write_fd; /* 和master线程通信管道写入端 */
}msd_thread_signal_t;

int msd_init_public_signals();
int msd_init_private_signals();

#endif /* __MSD_SIGNAL_H_INCLUDED__ */

