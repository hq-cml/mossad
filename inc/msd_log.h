/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_log.h
 * 
 * Description :  Msd_log, a generic log implementation.
 *                两个版本的日志：进程版本和线程版本。
 *                当mossad采用多进程时，用进程版本；采用多线程时，用线程版本
 *                对外接口都是统一的，只需要在msd_core.h中定义需要类型的宏
 *                #define MSD_LOG_MODE_THREAD(默认)
 *                #define MSD_LOG_MODE_PROCESS
 *
 *     Created :  Apr 6, 2014 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/

#ifndef _MSD_H_LOG_INCLUDED_
#define _MSD_H_LOG_INCLUDED_

/*
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
       
#define MSD_PTHREAD_LOCK_MODE
#define MSD_LOG_MODE_THREAD
#include "msd_lock.h"
*/

#define MSD_OK       0
#define MSD_ERR     -1
#define MSD_FAILED  -2
#define MSD_NONEED  -3

/* log level */
#define MSD_LOG_LEVEL_FATAL     0    
#define MSD_LOG_LEVEL_ERROR     1    
#define MSD_LOG_LEVEL_WARNING   2    
#define MSD_LOG_LEVEL_NOTICE    3    
#define MSD_LOG_LEVEL_DEBUG     4    
#define MSD_LOG_LEVEL_ALL       MSD_LOG_LEVEL_DEBUG

/* log size and num */
#define MSD_LOG_FILE_SIZE       (1<<30) /* 2G */
#define MSD_LOG_MULTIMODE_NO    0
#define MSD_LOG_MULTIMODE_YES   1
#define MSD_LOG_FILE_NUM        20

/* screen */
#define MSD_SCREEN_COLS         80
#define MSD_CONTENT_COLS        65

/* length */
#define MSD_LOG_BUFFER_SIZE     4096
#define MSD_LOG_MAX_NUMBER      100
#define MSD_LOG_PATH_MAX        1024

/*
 * isatty - test whether a file descriptor refers to a terminal, returns 1 if 
 * fd is an open file descriptor referring to a terminal; otherwise  0 is returned,
 * and errno is set to indicate the error.
 * 如果STDOUT_FILENO定向至终端，则以有色形式删除ok/failed，否则以普通形式输出
 */
/* print [OK] in green */
#define MSD_OK_STATUS \
    (isatty(STDOUT_FILENO)? \
     ("\033[1m[\033[32m OK \033[37m]\033[m") : ("[ OK ]"))
     
/* print [FAILED] in red */
#define MSD_FAILED_STATUS \
    (isatty(STDOUT_FILENO)? \
     ("\033[1m[\033[31m FAILED \033[37m]\033[m") :("[ FAILED ]"))


/*
 * C的宏中:
 * #的功能是将其后面的宏参数进行字符串化操作（Stringfication），
 * 简单说就是在对它所引用的宏 变量通过替换后在其左右各加上一个双引号。
 *
 * ##被称为连接符（concatenator），用来将两个Token连接为一个Token，
 * 或者充当的作用就是当变参为空的时候，消除前面的那个逗号,即Variadic Macro，也就是变参宏
 * 比如：
 * #define myprintf(templt,...) fprintf(stderr,templt,__VA_ARGS__) 或者 
 * #define myprintf(templt,args...) fprintf(stderr,templt,args)
 * 第一个宏中由于没有对变参起名，我们用默认的宏__VA_ARGS__来替代它。第二个宏中，
 * 我们显式地命名变参为args，那么我们在宏定义中就可以用args来代指变参了。 
 * 同C语言的stdcall一样，变参必须作为参数表的最有一项出现。
 *
 * 若变参为空，会导致编译错误，此时
 * #define myprintf(templt, ...) fprintf(stderr,templt, ##__VAR_ARGS__)
 * 这时，##这个连接符号充当的作用就是当__VAR_ARGS__为空的时候，消除前面的那个逗号。那么此时的翻译过程如下：
 * myprintf(templt); ==> myprintf(stderr,templt);
 */
#define MSD_BOOT_OK(fmt, args...) msd_boot_notify(0, fmt, ##args)
#define MSD_BOOT_FAILED(fmt, args...) do {\
    msd_boot_notify(-1, fmt, ##args); \
    exit(1);\
}while(0)

#define MSD_DETAIL(level, fmt, args...) \
    msd_log_write(level, "[%s:%d:%s]" fmt, __FILE__, __LINE__, __FUNCTION__, ##args) /* whether add #?? */

#define MSD_FATAL_LOG(fmt, args...) MSD_DETAIL(MSD_LOG_LEVEL_FATAL, fmt, ##args)
#define MSD_ERROR_LOG(fmt, args...) MSD_DETAIL(MSD_LOG_LEVEL_ERROR, fmt, ##args)
#define MSD_WARNING_LOG(fmt, args...) MSD_DETAIL(MSD_LOG_LEVEL_WARNING, fmt, ##args)
#define MSD_NOTICE_LOG(fmt, args...) MSD_DETAIL(MSD_LOG_LEVEL_NOTICE, fmt, ##args)
#define MSD_DEBUG_LOG(fmt, args...) MSD_DETAIL(MSD_LOG_LEVEL_DEBUG, fmt, ##args)

void msd_boot_notify(int ok, const char *fmt, ...);
int msd_log_init(const char *dir, const char *filename, int level, int size, int lognum, int multi);
int msd_log_write(int level, const char *fmt, ...);
void msd_log_close();

/* static int msd_get_map_mem(); */
/* static int msd_log_rotate(int fd, const char *path); */
#endif /*_MSD_H_LOG_INCLUDED_*/

