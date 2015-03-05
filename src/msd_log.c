/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_log.c
 * 
 * Description :  Msd_log, a generic log implementation.
 *                两个版本的日志：进程版本和线程版本。
 *                当mossad采用多进程时，用进程版本；采用多线程时，用线程版本
 *                对外接口都是统一的，只需要在Makefile中定义需要类型的宏
 *                LOG_MODE = -D__MSD_LOG_MODE_THREAD__/-D__MSD_LOG_MODE_PROCESS__
 *
 *
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/
#include "msd_core.h"

static char *msd_log_level_name[] = { /* char **msd_log_level_name */
    "FATAL",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG"
};

static msd_log_t g_log; /* 全局Log句柄 */

/**
 * 功能: 开辟一块共享内存(废弃!)
 * 描述:
 *      1. 匿名方式打开共享内存，不需要映射文件
 *      2. MAP_PRIVATE，进程的私有的共享内存，即便其可能派生了子进程，父子打印出来的msd_log_buffer
 *         的值都是相同的，但他们确实私有的，互不影响
 *      3. MAP_SHARED，真正能够父子进程互相共享
 *      4. 经过测试，在多线程模式下，共享内存的方式会发生错乱，所以不使用共享内存模式
 * 返回: 成功，0 失败，-x

static int msd_get_map_mem()
{
    if(msd_log_buffer == MAP_FAILED)
    {
        //printf("mmap is called by pid %d\n", getpid());
        msd_log_buffer = mmap(0, MSD_LOG_BUFFER_SIZE, PROT_WRITE | PROT_READ,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                //MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (msd_log_buffer == MAP_FAILED)
        {
            fprintf(stderr, "mmap log buffer failed: %s\n",strerror(errno));
            return MSD_FAILED;
        }
    }
    return MSD_OK;
}
 **/
 
/**
 * 功能: 根据ok的值，输出红色或者绿色在屏幕，最长不超过80字符
 **/
void msd_boot_notify(int ok, const char *fmt, ...)
{
    va_list  ap;
    int      n;

    char log_buffer[MSD_LOG_BUFFER_SIZE] = {0};
    va_start(ap, fmt);
    n = vsnprintf(log_buffer, MSD_SCREEN_COLS, fmt, ap);/*n 是成功写入的字符数*/
    va_end(ap);

    if(n > MSD_CONTENT_COLS)
    {
        printf("%-*.*s%s%s\n", MSD_CONTENT_COLS-5, MSD_CONTENT_COLS-5,/*其中前边*定义的是总的宽度，后边*是指定输出字符个数。*/
                log_buffer, " ... ", ok==0? MSD_OK_STATUS:MSD_FAILED_STATUS);
    }
    else
    {
        printf("%-*.*s%s\n", MSD_CONTENT_COLS, MSD_CONTENT_COLS,
                log_buffer, ok==0? MSD_OK_STATUS:MSD_FAILED_STATUS);
    }
}

/**
 * 功能: 重置日志文件fd
 * 参数: @index
 * 返回: 成功，0 失败，-x
 **/
static int msd_log_reset_fd(int index)
{
    int status;
    if(g_log.g_msd_log_files[index].fd != -1)
    {
        close(g_log.g_msd_log_files[index].fd);
    }
    g_log.g_msd_log_files[index].fd = open(g_log.g_msd_log_files[index].path,
            O_WRONLY|O_CREAT|O_APPEND, 0644);
    if(g_log.g_msd_log_files[index].fd < 0)
    {
        fprintf(stderr, "open log file %s failed: %s\n",
               g_log.g_msd_log_files[index].path, strerror(errno));
        return MSD_FAILED;
    }

    /* in case exec.. */
    /*如果子进程被exec族函数替换，则他是无法操作该文件了*/
    status = fcntl(g_log.g_msd_log_files[index].fd, F_GETFD, 0);
    status |= FD_CLOEXEC;
    fcntl(g_log.g_msd_log_files[index].fd, F_SETFD, status);  
    return MSD_OK;
}

/**
 * 功能: log init
 * 参数: @dir 目录位置
 *       @filename 文件名
 *       @level 
 *       @size
 *       @lognum
 *       @multi
 * 描述:
 *      1. access(dir, mode)
 *         mode:表示测试的模式可能的值有:
 *              R_OK:是否具有读权限             
 *              W_OK:是否具有可写权限
 *              X_OK:是否具有可执行权限             
 *              F_OK:文件是否存在             
 *              返回值:若测试成功则返回0,否则返回-1
 *  
 * 返回: 成功，0 失败，-x
 **/
int msd_log_init(const char *dir, const char *filename, int level, int size, int lognum, int multi)
{
    assert(dir && filename);
    assert(level >= MSD_LOG_LEVEL_FATAL && level <= MSD_LOG_LEVEL_ALL);
    assert(size >= 0 && size <= MSD_LOG_FILE_SIZE);
    assert(lognum > 0 && lognum <= MSD_LOG_MAX_NUMBER);

    if(g_log.msd_log_has_init)
    {
        return MSD_ERR;
    }
    
    /* whether can be wirten */
    if (access(dir, W_OK)) 
    {
        fprintf(stderr, "access log dir %s failed:%s\n", dir, strerror(errno));
        return MSD_ERR;
    }

    g_log.msd_log_level = level;
    g_log.msd_log_size  = size;
    g_log.msd_log_num   = lognum;
    g_log.msd_log_multi = multi; /* !!multi */

    /* 初始化roate锁 */
    if (MSD_LOCK_INIT(g_log.msd_log_rotate_lock) != 0) 
    {
        return MSD_ERR;
    }
    
    if(g_log.msd_log_multi)/* multi log mode */
    {
        int i;
        for(i=0; i<=MSD_LOG_LEVEL_DEBUG; i++)
        {
            g_log.g_msd_log_files[i].fd = -1;
            memset(g_log.g_msd_log_files[i].path, 0, MSD_LOG_PATH_MAX);
            strcpy(g_log.g_msd_log_files[i].path, dir);
            if(g_log.g_msd_log_files[i].path[strlen(dir)] != '/')
            {
                strcat(g_log.g_msd_log_files[i].path,  "/");
            }
            strcat(g_log.g_msd_log_files[i].path, filename);
            strcat(g_log.g_msd_log_files[i].path, "_");
            strcat(g_log.g_msd_log_files[i].path, msd_log_level_name[i]);

            /*初始化fd*/
            if(MSD_OK != msd_log_reset_fd(i))
            {
                return MSD_FAILED;
            }
        }
    }
    else
    {
        g_log.g_msd_log_files[0].fd = -1;
        memset(g_log.g_msd_log_files[0].path, 0, MSD_LOG_PATH_MAX);
        strcpy(g_log.g_msd_log_files[0].path, dir);
        if(g_log.g_msd_log_files[0].path[strlen(dir)] != '/')
        {
            strcat(g_log.g_msd_log_files[0].path, "/");
        }
        strcat(g_log.g_msd_log_files[0].path, filename);

        /*初始化fd*/
        if(MSD_OK != msd_log_reset_fd(0))
        {
            return MSD_FAILED;
        }
    }

    /*
    if(msd_get_map_mem())
    {
        return MSD_ERR;
    }
    */

    g_log.msd_log_has_init = 1;
    return MSD_OK;
}

/**
 * 功能: close log
 **/
void msd_log_close()
{
    int i;
    if(g_log.msd_log_multi)
    {
        for(i=0; i<=MSD_LOG_LEVEL_DEBUG; i++)
        {
            if(g_log.g_msd_log_files[i].fd != -1)
            {
                close(g_log.g_msd_log_files[i].fd);
                g_log.g_msd_log_files[i].fd = -1;
            }
        }
    }
    else
    {
        if(g_log.g_msd_log_files[0].fd != -1)
        {
            close(g_log.g_msd_log_files[0].fd);
            g_log.g_msd_log_files[0].fd = -1;
        }
    }
    
    MSD_LOCK_DESTROY(g_log.msd_log_rotate_lock); /* 消除锁 */
}

#ifdef MSD_LOG_MODE_PROCESS
/**
 * 功能: 日志切割，进程版本
 * 参数: @
 * 描述:
 *      1. rotate的时候，加锁保护，防止紊乱
 *      2. 为了保证多进程模式下的可靠，分别采用了fstat和stat
 * 返回: 成功，0 失败，-x
 **/
static int msd_log_rotate(int fd, const char* path, int level)
{
    char tmppath1[MSD_LOG_PATH_MAX];
    char tmppath2[MSD_LOG_PATH_MAX];
    int  i;
    struct stat st;
    int index;
    
    index = g_log.msd_log_multi? level:0;

    /*
     * 注意!
     * 此处须用fstat，而下面那处须用stat。为了兼容多进程的情况，此处须用fd，因为别的进程有可能
     * 已经rotate了日志文件，如果用了path，就可能检测出大小没有超过阈值(因为别的进程已rotate)，
     * 但是写入的时候用的是fd，这就出了问题，因为该进程fd指向的已经是xxx.log.max，而不再是xxx.log
     * ，所以日志会写到已经rotate了的xx.log.max当中去。但是如果此处用fd，就不会有这种问题，因为即
     * 便文件名字变化了，但是fd仍然指向的是原来的文件，这样仍然判断test.log.max，仍然可以发现日志超标
     **/
    /* get the file satus, store in st */
    if(MSD_OK != fstat(fd, &st))
    {
        /*若出现异常，则尝试重置fd，无论是否成功，返回FAILED*/
        //MSD_LOCK_LOCK(g_log.msd_log_rotate_lock);
        msd_log_reset_fd(index);
        //MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
        return MSD_FAILED;
    }

    if(st.st_size >= g_log.msd_log_size)
    {
        //加锁
        MSD_LOCK_LOCK(g_log.msd_log_rotate_lock);
        printf("process %d get the lock\n", getpid());
        //sleep(5);
        /*
         * 注意!
         * 再次判断是否超标，此处必须用stat，而上面那处必须用fstat，因为如果用了fd，必然检测出日志超标
         * ，毕竟上面已经检测过了一次。此时有可能其他进程已经完成了roate，所以得用path再来判断一次，则
         * 1.如果仍然超标: 说明没有其他进程rotate过，那本进程负责rotate
         * 2.如果不再超标: 说明其他进程已经完成了roate，本进程fd已经指向了xx.log.max，
         *   则本进程只需要更新自己的fd
         **/
        if(MSD_OK != stat(path, &st))
        {
            printf("process %d relase the lock, stat error\n", getpid());
            perror("the reason of the error:");
            msd_log_reset_fd(index);
            MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
            return MSD_FAILED;
        }
        
        /*
         * 如果此刻发现，日志文件已经不符合roate的条件了，说明了已经有其他进程rotate了，
         * 则此刻应该返回OK或者NONEED并解锁，然后重置自己的fd
         **/        
        if(st.st_size < g_log.msd_log_size)
        {
            close(g_log.g_msd_log_files[index].fd);
            g_log.g_msd_log_files[index].fd = -1;

            if(MSD_OK != msd_log_reset_fd(index))
            {
                MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
                return MSD_FAILED;               
            }
            
            printf("process %d relase the lock,ohter process roate\n", getpid());
            /*
             * 两种思路:经测试发现概率上第一种效果较好
             * 1.继续加锁，返回OK，等自己写完之后解锁
             * 2.直接解锁，返回NONEED，然后由write函数写入  
             */
            /*交由wirte函数写入完毕后再释放*/
            //MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
            //return MSD_NONEED;
            return MSD_OK;
        }
        
        /* 仍然超标，说明其他进程没有进行rotate，则由本进程完成 */
        /* find the first not exist file name */
        for(i = 0; i < g_log.msd_log_num; i++)
        {
            snprintf(tmppath1, MSD_LOG_PATH_MAX, "%s.%d", path, i);
       
            /* whether exist */        
            if(access(tmppath1, F_OK))
            {
                /* the file tmppath1 not exist */
                rename(path, tmppath1);/*rename(from, to)*/
                printf("process %d find the unexist file\n", getpid());

                close(g_log.g_msd_log_files[index].fd);
                g_log.g_msd_log_files[index].fd = -1;

                if(MSD_OK != msd_log_reset_fd(index))
                {
                    MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
                    return MSD_FAILED;               
                }
                
                /* 我实施了rotate，则不应该解锁，应该等待我写完了，才解锁 */
                //MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
                //sleep(5);
                return MSD_OK;
            }
        }
        
        /* all path.n exist, then ,rotate */
        /* 如果日志数已经达到上限，则会将所有的带后缀的日志整体前移，并将当前日志，以最大后缀命名 */
        for(i=1; i<g_log.msd_log_num; i++)
        {
            snprintf(tmppath1, MSD_LOG_PATH_MAX, "%s.%d", path, i);
            snprintf(tmppath2, MSD_LOG_PATH_MAX, "%s.%d", path, i-1);
            /* rename file.(delete the file.n which n is low) */
            rename(tmppath1, tmppath2);
        }

        /*将当前日志，以最大后缀命名*/
        snprintf(tmppath2, MSD_LOG_PATH_MAX, "%s.%d", path, g_log.msd_log_num-1);
        rename(path, tmppath2);

        close(g_log.g_msd_log_files[index].fd);
        g_log.g_msd_log_files[index].fd = -1;
        
        if(MSD_OK != msd_log_reset_fd(index))
        {
            MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
            return MSD_FAILED;               
        }
        
        printf("process %d roate the file\n", getpid());
        /*我实施了rotate，则不应该解锁，应该等待我写完了，才解锁*/
        //MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
        //sleep(2);
        return MSD_OK;

    }
    
    return MSD_NONEED;

}
/**
 * 功能: 日志写入，进程版本
 * 参数: @level， @fmt , @...
 * 描述:
 *      1. 每次写入前，会触发rotate检查 
 * 返回: 成功，0 失败，-x
 **/
int msd_log_write(int level, const char *fmt, ...)
{
    struct tm   tm; /* time struct */
    int         pos;
    int         end;
    va_list     ap;
    time_t      now;
    int         index;
    int         rotate_result;
    char        log_buffer[MSD_LOG_BUFFER_SIZE] = {0};
    
    if(!g_log.msd_log_has_init)
    {
        return MSD_ERR;
    }

    /* only write the log whose levle low than system initial log level */
    if(level > g_log.msd_log_level)
    {
        return MSD_NONEED;
    }
    /*
    if(msd_get_map_mem() != MSD_OK)
    {
        return MSD_FAILED;
    }
    */
    now = time(NULL);
    /*localtime_r() 函数将日历时间timep转换为用户指定的时区的时间。但是它可以将数据存储到用户提供的结构体中。*/
    localtime_r(&now, &tm);

    pos = sprintf(log_buffer, "[%04d-%02d-%02d %02d:%02d:%02d][%05d][%5s]",
                tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec, getpid(),
                msd_log_level_name[level]);

    /*加工日志内容*/
    va_start(ap, fmt);
    end = vsnprintf(log_buffer+pos, MSD_LOG_BUFFER_SIZE-pos, fmt, ap);
    va_end(ap);

    /* 越界处理 */
    if(end >= (MSD_LOG_BUFFER_SIZE-pos))
    {
        log_buffer[MSD_LOG_BUFFER_SIZE-1] = '\n';
        memset(log_buffer+MSD_LOG_BUFFER_SIZE-4, '.', 3);
        end = MSD_LOG_BUFFER_SIZE-pos-1;
    }
    else
    {
        log_buffer[pos+end] = '\n';
    }    

    index = g_log.msd_log_multi? level:0;

    /*异常处理*/
    if(g_log.g_msd_log_files[index].fd == -1)
    {
        //MSD_LOCK_LOCK(g_log.msd_log_rotate_lock);
        if(MSD_OK == msd_log_reset_fd(index))
        {
            return MSD_FAILED;
        }
        //MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
    }


    if(MSD_NONEED == (rotate_result = msd_log_rotate(g_log.g_msd_log_files[index].fd, (const char*)g_log.g_msd_log_files[index].path, level)))
    {
        if(write(g_log.g_msd_log_files[index].fd, log_buffer, end + pos + 1) != (end + pos + 1))/* +1 是为了'\n' */            
        {
            fprintf(stderr,"write log to file %s failed: %s\n", g_log.g_msd_log_files[index].path, strerror(errno));
            return MSD_FAILED;
        }
    }
    else if(MSD_OK == rotate_result)
    {
        /* 返回MSD_OK 说明是由本进程执行了roate，或者在处于锁定状态中的时候，别的进程完成了roate，则应该在完成了写入操作之后再解锁 */
        if(write(g_log.g_msd_log_files[index].fd, log_buffer, end + pos + 1) != (end + pos + 1))       
        {
            fprintf(stderr,"write log to file %s failed: %s\n", g_log.g_msd_log_files[index].path, strerror(errno));
            return MSD_FAILED;
        }
        /*解锁*/
        printf("process %d relase the lock\n", getpid());
        MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);        
    }
    else
    {
        /*do nothing*/
        return MSD_FAILED;
    }

    return MSD_OK;
}
#else

/**
 * 功能: 日志切割，线程版本
 * 参数: @
 * 描述:
 *      1. rotate的时候，加锁保护，防止紊乱
 * 返回: 成功，0 失败，-x
 **/
static int msd_log_rotate(int fd, const char* path, int level)
{
    char tmppath1[MSD_LOG_PATH_MAX];
    char tmppath2[MSD_LOG_PATH_MAX];
    int  i;
    struct stat st;
    int index;
    
    index = g_log.msd_log_multi? level:0;

    /* get the file satus, store in st */
    if(MSD_OK != fstat(fd, &st))
    {
        /* 若出现异常，则尝试重置fd，无论是否成功，返回FAILED
         * 不加锁，测试中发现，会发生自己把自己锁住
         */
        //MSD_LOCK_LOCK(g_log.msd_log_rotate_lock);
        msd_log_reset_fd(index);
        //MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
        return MSD_FAILED;
    }

    if(st.st_size >= g_log.msd_log_size)
    {
        //加锁
        MSD_LOCK_LOCK(g_log.msd_log_rotate_lock);
        printf("thread %lu get the lock\n", (unsigned long)pthread_self());
        //sleep(5);
        /*
         * 注意!
         * 再次判断是否超标，此处有可能其他进程已经完成了roate，所以再判断一次，则
         * 1.如果仍然超标: 说明没有其线程rotate过，那本进程负责rotate
         * 2.如果不再超标: 说明其他线程已经完成了roate，fd已经更新，则直接退出
         **/
        if(MSD_OK != fstat(fd, &st))
        {
            printf("thread %lu relase the lock, stat error\n", (unsigned long)pthread_self());
            perror("the reason of the error:");
            msd_log_reset_fd(index);
            MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
            return MSD_FAILED;
        }
        
        /* 说明其他线程已经完成了roate，fd已经更新，则直接退出 */        
        if(st.st_size < g_log.msd_log_size)
        {
        
            printf("thread %lu relase the lock,ohter thread roate\n", (unsigned long)pthread_self());
            MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
            return MSD_NONEED;
        }
        
        /* 仍然超标，说明其他线程没有进行rotate，则由本进程完成 */
        /* find the first not exist file name */
        for(i = 0; i < g_log.msd_log_num; i++)
        {
            snprintf(tmppath1, MSD_LOG_PATH_MAX, "%s.%d", path, i);
       
            /* whether exist */        
            if(access(tmppath1, F_OK))
            {
                /* the file tmppath1 not exist */
                rename(path, tmppath1);/*rename(from, to)*/
                printf("thread %lu find the unexist file\n", (unsigned long)pthread_self());

                close(g_log.g_msd_log_files[index].fd);
                g_log.g_msd_log_files[index].fd = -1;

                if(MSD_OK != msd_log_reset_fd(index))
                {
                    MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
                    return MSD_FAILED;               
                }

                MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
                //sleep(5);
                return MSD_OK;
            }
        }
        
        /* all path.n exist, then ,rotate */
        /* 如果日志数已经达到上限，则会将所有的带后缀的日志整体前移，并将当前日志，以最大后缀命名 */
        for(i=1; i<g_log.msd_log_num; i++)
        {
            snprintf(tmppath1, MSD_LOG_PATH_MAX, "%s.%d", path, i);
            snprintf(tmppath2, MSD_LOG_PATH_MAX, "%s.%d", path, i-1);
            /* rename file.(delete the file.n which n is low) */
            rename(tmppath1, tmppath2);
        }

        /*将当前日志，以最大后缀命名*/
        snprintf(tmppath2, MSD_LOG_PATH_MAX, "%s.%d", path, g_log.msd_log_num-1);
        rename(path, tmppath2);

        close(g_log.g_msd_log_files[index].fd);
        g_log.g_msd_log_files[index].fd = -1;
        
        if(MSD_OK != msd_log_reset_fd(index))
        {
            MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
            return MSD_FAILED;               
        }
        
        printf("thread %lu roate the file\n", (unsigned long)pthread_self());
        MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
        //sleep(2);
        return MSD_OK;

    }
    
    return MSD_NONEED;

}
/**
 * 功能: 日志写入，线程版本
 * 参数: @level， @fmt , @...
 * 描述:
 *      1. 每次写入前，会触发rotate检查 
 * 返回: 成功，0 失败，-x
 **/
int msd_log_write(int level, const char *fmt, ...)
{
    struct tm   tm; /* time struct */
    int         pos;
    int         end;
    va_list     ap;
    time_t      now;
    int         index;
    int         rotate_result;
    char        log_buffer[MSD_LOG_BUFFER_SIZE] = {0};
    
    if(!g_log.msd_log_has_init)
    {
        return MSD_ERR;
    }

    /* only write the log whose level low than system initial log level */
    if(level > g_log.msd_log_level)
    {
        return MSD_NONEED;
    }
    /*
    if(msd_get_map_mem() != MSD_OK)
    {
        return MSD_FAILED;
    }
    */ 
    now = time(NULL);
    /*localtime_r() 函数将日历时间timep转换为用户指定的时区的时间。但是它可以将数据存储到用户提供的结构体中。*/
    localtime_r(&now, &tm);

    pos = sprintf(log_buffer, "[%04d-%02d-%02d %02d:%02d:%02d][%lu][%5s]",
                tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec, pthread_self(),
                msd_log_level_name[level]);

    /*加工日志内容*/
    va_start(ap, fmt);
    end = vsnprintf(log_buffer+pos, MSD_LOG_BUFFER_SIZE-pos, fmt, ap);
    va_end(ap);

    /* 越界处理 */
    if(end >= (MSD_LOG_BUFFER_SIZE-pos))
    {
        log_buffer[MSD_LOG_BUFFER_SIZE-1] = '\n';
        memset(log_buffer+MSD_LOG_BUFFER_SIZE-4, '.', 3);
        end = MSD_LOG_BUFFER_SIZE-pos-1;
    }
    else
    {
        log_buffer[pos+end] = '\n';
    }    

    index = g_log.msd_log_multi? level:0;

    /*异常处理*/
    if(g_log.g_msd_log_files[index].fd == -1)
    {
        //MSD_LOCK_LOCK(g_log.msd_log_rotate_lock);
        if(MSD_OK == msd_log_reset_fd(index))
        {
            return MSD_FAILED;
        }
        //MSD_LOCK_UNLOCK(g_log.msd_log_rotate_lock);
    }


    if(MSD_NONEED == (rotate_result = msd_log_rotate(g_log.g_msd_log_files[index].fd, (const char*)g_log.g_msd_log_files[index].path, level)))
    {
        if(write(g_log.g_msd_log_files[index].fd, log_buffer, end + pos + 1) != (end + pos + 1))/* +1 是为了'\n' */            
        {
            fprintf(stderr,"write log to file %s failed: %s\n", g_log.g_msd_log_files[index].path, strerror(errno));
            return MSD_FAILED;
        }
    }
    else if(MSD_OK == rotate_result)
    {
        /* 返回MSD_OK 说明是由本进程执行了roate，则写入操作 */
        if(write(g_log.g_msd_log_files[index].fd, log_buffer, end + pos + 1) != (end + pos + 1))       
        {
            fprintf(stderr,"write log to file %s failed: %s\n", g_log.g_msd_log_files[index].path, strerror(errno));
            return MSD_FAILED;
        }      
    }
    else
    {
        /*do nothing*/
        return MSD_FAILED;
    }

    return MSD_OK;
}

#endif
#ifdef __MSD_LOG_TEST_MAIN__

void test(void *arg)
{
    int j=0;
    for(j=0; j < 1; j++)
    {
        MSD_FATAL_LOG("%s", "chil"); //一行是60个字节
    }
}

int main()
{
    /******** 普通测试 **********/
    /*
    printf("%s\n",MSD_OK_STATUS);
    printf("%s\n",MSD_FAILED_STATUS);

    msd_boot_notify(0,"hello %s","world");
    msd_boot_notify(0,"hello world hello world hello world hello world hello world hello world hello world hello world");
    msd_boot_notify(1,"hello world hello world hello world hello world hello world hello world hello world hello world");
    
 
    MSD_BOOT_SUCCESS("This is ok:%s", "we'll continue");
    MSD_BOOT_SUCCESS("This is ok:%s", "This is a very long message which will extended the limit of the screen");
    */
    
    /* 
    msd_log_init("./logs","test.log", MSD_LOG_LEVEL_ALL, 1024*1024*256, 3, 0);
 
    //msd_log_init("./logs","test.log", MSD_LOG_LEVEL_ALL, 1024*1024*256, 3, 1); 
    MSD_FATAL_LOG("Hello %s", "world!");
    MSD_ERROR_LOG("Hello %s", "birth!");
    msd_log_write(MSD_LOG_LEVEL_DEBUG, "hei %s", "a!");
    msd_log_write(MSD_LOG_LEVEL_NOTICE, "hello %s", "b!");
    msd_log_write(MSD_LOG_LEVEL_DEBUG, "morning %s", "again!");
    */
    /*
    srand(time(NULL));
    int i=0;
    while(1)
    {
        msd_log_write(rand()%(MSD_LOG_LEVEL_DEBUG +1),
            "[%s:%d]%s:%d", __FILE__, __LINE__, "Test mossad log", i++);
    }
    */

    /**************测试rotate在多进程模式下的切换紊乱问题**************/  
    /*
     * 用例1:文件大小限制为6000，log_num=9，1000个进程并发写入60字节
     *       结果完美写入了10个文件，无错乱，无丢失，可以看看，和原始
     *       版本比较非常明显
     */
    /*
     * 用例2:文件大小限制为60000，log_num=9，40个进程并发写入600000字节
     *       多进程版本时而出现文件略大的问题，但是日志总数量稳定。原始
     *       版本日志文件大小没谱，日志数量也没什么谱
     */
    /*
     * 用例3:文件大小限制为60000，log_num=9，1000个进程并发写入600000字节
     *       多进程版本能够勉强应付，有轻微的超额写入的问题，但是基本日志数量 
     *       等都很稳定，一直保持在10个。原始版本没法看了，单个日志大小以及
     *       日志数量两个指标都没谱
     */
    /*
     * 用例4:文件大小限制为1G，log_num=9，1000个进程并发写入6000000字节
     *       多进程版本也有点扛不住，日志roate后大概1.1G左右，原始版本就不测了  
     */
    /*
     * 用例5:文件大小限制为6000000，log_num=9，40个进程并发写入600000*2字节
     *       测试多进程版本中当解锁后发现其他进程rotate了日志，有两种思路
     *       1.继续加锁，返回OK，等自己写完之后解锁
     *       2.直接解锁，返回NONEED，然后写入  
     *       结论第一种概率上较好
     */
    /*
    MSD_BOOT_SUCCESS("the programe start");
    msd_log_init("./logs","test.log", MSD_LOG_LEVEL_ALL, 6000, 9, 0);

    int i = 0;
    int child_cnt = 1000;
    //int child_cnt = 40;
    printf("father start! pid: %d\n", getpid());
    for(i=0; i<child_cnt; i++)
    {
        pid_t pid;
        if((pid = fork()) == 0)
        {
            //printf("I am child:%d \n", getpid());
            int j=0;
            for(j=0; j < 1; j++)
            {
                MSD_FATAL_LOG("%s", "chil"); //一行是60个字节
                //MSD_FATAL_LOG("%s", "chil");
            }
            exit(0);
        }
        else if(pid>0)
        {
            printf("spawn child %d\n", pid);
        }
        else
        {
            printf("spawn error %d\n", pid);
            perror("");
        }
    }

    //父进程等待子进程退出
    int status;
    for(i=0; i<child_cnt; i++)
    {
        waitpid( -1, &status, 0 );
    }
    printf("father exit! pid: %d, the child cnt: %d\n", getpid(), child_cnt);
 
    msd_log_close();

    if(0 > execl("/bin/ls", "ls", "-l" ,"/home/huaqi/mossad/test/log/logs/", NULL))
    {
        perror("ls error");
    }
    */
    /**************测试rotate在多线程模式下的切换紊乱问题**************/ 
    /*
     * 用例1,2,3,4: 和第一个进程的用例相同，换成线程
     * 结论：在极端情况下面，也不能完全保证每份日志大小，但是基本能够稳定。日志数量大小
     *       可以稳定保证
     */
    MSD_BOOT_SUCCESS("the programe start");
    msd_log_init("./logs","test.log", MSD_LOG_LEVEL_ALL, 1<<30, 9, 0);    
    int i;
    int child_cnt = 1000;
    //int child_cnt = 40;    
    pthread_t thread[child_cnt];               /*保存线程号*/

    for(i=0; i<child_cnt; i++)
    {
        pthread_create(&thread[i], NULL, (void *)test, NULL);
        //printf("create thread %ul\n", (unsigned long )thread[i]);
    }    
    //join
    for(i=0; i<child_cnt; i++)
    {
        pthread_join(thread[i], NULL);
    }  
    
    msd_log_close();

    if(0 > execl("/bin/ls", "ls", "-l" ,"/home/huaqi/mossad/test/log/logs/", NULL))
    {
        perror("ls error");
    }    
    
    
    return 0;
}
#endif /* __MSD_LOG_TEST_MAIN__ */

