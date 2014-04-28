/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_lock.c
 * 
 * Description :  Msd_lock, a generic lock implementation.
 *                提供三种锁类型：pthread_mutex, semaphore, fcntl，三种锁型
 *                的对外接口都是统一的，用户只需要在msd_core.h中定义需要类型的宏
 *                #define   MSD_PTHREAD_LOCK_MODE(推荐)
 *                #define   MSD_SYSVSEM_LOCK_MODE
 *                #define   MSD_FCNTL_LOCK_MODE(仅适用于进程)
 *
 *                其中pthread_mutex模型在在多进程的情况下，需要将所建立在共享内存
 *                才能生效
 *
 *                适用范围：
 *                进程：MSD_PTHREAD_LOCK_MODE/MSD_SYSVSEM_LOCK_MODE/MSD_FCNTL_LOCK_MODE
 *                线程：MSD_PTHREAD_LOCK_MODE/MSD_SYSVSEM_LOCK_MODE
 *
 *     Created :  Apr 5, 2012
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/
#include "msd_core.h"

#if defined(MSD_PTHREAD_LOCK_MODE)

/**
 * 功能: Lock implementation based on pthread mutex.
 * 参数: @ppl
 * 描述:
 *      1. 共享内存之上，生成lock，父子共享之，多进程的时候必须是父子共享锁，否则无法实现锁功能
 *      2. 设置互斥锁生效范围:互斥锁变量可以是进程专用的（进程内）变量，也可以是系统范围内的
 *         （进程间）变量。要在多个进程中的线程之间共享互斥锁，可以在共享内存中创建互斥锁，并将 
 *         pshared 属性设置为 PTHREAD_PROCESS_SHARED。 
 * 返回: 成功，0 失败，-x
 **/
int msd_pthread_lock_init(msd_lock_t **ppl) 
{
    pthread_mutexattr_t     mattr;
    
	/* 共享内存之上，生成lock */
    *ppl = mmap(0, sizeof(msd_lock_t), PROT_WRITE | PROT_READ,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (*ppl == MAP_FAILED)
    {
        fprintf(stderr, "mmap lock failed: %s\n",strerror(errno));
        return MSD_ERR;
    }

    if (pthread_mutexattr_init(&mattr) != 0) 
    {
        return MSD_ERR;
    }

    /* 设置互斥锁范围: PTHREAD_PROCESS_SHARED */
    if (pthread_mutexattr_setpshared(&mattr, 
            PTHREAD_PROCESS_SHARED) != 0) 
    {
        pthread_mutexattr_destroy(&mattr);
        return MSD_ERR;
    }

    if (pthread_mutex_init(&((*ppl)->mutex), &mattr) != 0) 
    {
        pthread_mutexattr_destroy(&mattr);
        munmap(*ppl, sizeof(msd_lock_t));
        return MSD_ERR;
    }
    pthread_mutexattr_destroy(&mattr);
    //printf("Thread lock init\n");
    return MSD_OK;
}

/**
 * 功能: lock
 * 参数: @pl
 * 返回: 成功，0 失败，-x
 **/
int msd_pthread_do_lock(msd_lock_t *pl) 
{
    if (pthread_mutex_lock(&(pl->mutex)) != 0) 
    {
        return MSD_ERR;
    }
    return MSD_OK;
}

/**
 * 功能: unlock
 * 参数: @pl
 * 返回: 成功，0 失败，-x
 **/
int msd_pthread_do_unlock(msd_lock_t *pl) 
{
    if (pthread_mutex_unlock(&(pl->mutex)) != 0) 
    {
        return MSD_ERR;
    }
    return MSD_OK;
}

/**
 * 功能: destroy lock
 * 参数: @pl
 **/
void msd_pthread_lock_destroy(msd_lock_t *pl) 
{
    pthread_mutex_destroy(&(pl->mutex));
    munmap( pl, sizeof(msd_lock_t) );
}


#elif defined(MSD_SYSVSEM_LOCK_MODE)

#define PROJ_ID_MAGIC   0xCD
union semun
{ 
    int val;                  /* value for SETVAL */
    struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array;    /* array for GETALL, SETALL */
                              /* Linux specific part: */
    struct seminfo *__buf;    /* buffer for IPC_INFO */
}; 

/**
 * 功能: Lock implementation based on System V semaphore.
 * 参数: @ppl @pathname
 * 描述:
 *      1. 父子共享之。其实此处无论用MAP_SHARED或者MAP_PRIVATE都是可以的原因是信
 *         号量使用的原理是通过信号量的id。类比于pthread的模式，只能用MAP_SHARED，
 *         否则无法实现锁功能
 * 返回: 成功，0 失败，-x
 **/
int msd_sysv_sem_init(msd_lock_t **ppl, const char *pathname) 
{
    int rc;
    union semun arg;
    int semid;
    int oflag = IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR;
    key_t key = IPC_PRIVATE;
    
	/* 共享内存之上，生成lock */
    *ppl = mmap(0, sizeof(msd_lock_t), PROT_WRITE | PROT_READ,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (*ppl == MAP_FAILED)
    {
        fprintf(stderr, "mmap lock failed: %s\n",strerror(errno));
        return MSD_ERR;
    }
	
    if (pathname) 
    {
        key = ftok(pathname, PROJ_ID_MAGIC);
        if (key < 0) 
        {
            return MSD_ERR;
        }
    }
    /* 信号量生成 */
    if ((semid = semget(key, 1, oflag)) < 0) 
    {
        return MSD_ERR;
    }
    /* 信号量设置 */
    arg.val = 1;
    do 
    {
        rc = semctl(semid, 0, SETVAL, arg);
    } while (rc < 0);

    if (rc < 0) 
    {
        do 
        {
            semctl(semid, 0, IPC_RMID, 0);
        } while (rc < 0 && errno == EINTR);
        return MSD_ERR;
    }
    
    /* !! 将id赋值给ppl */
    (*ppl)->semid = semid;
    //printf("Sem lock init\n");
    return semid;
}

/**
 * 功能: lock
 * 参数: @semid
 * 返回: 成功，0 失败，-x
 **/
int msd_sysv_sem_lock(int semid) 
{
    int rc;
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = SEM_UNDO;
    /* P */
    rc = semop(semid, &op, 1);
    return rc;
}

/**
 * 功能: unlock
 * 参数: @semid
 * 返回: 成功，0 失败，-x
 **/
int msd_sysv_sem_unlock(int semid) 
{
    int rc;
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = SEM_UNDO;
    /* V */
    rc = semop(semid, &op, 1);
    return rc;
}

/**
 * 功能: destroy lock
 * 参数: @pl, @semid
 * 返回: 成功，0 失败，-x
 **/
void msd_sysv_sem_destroy(msd_lock_t *pl, int semid) 
{
    int rc;
    do 
    {
        rc = semctl(semid, 0, IPC_RMID, 0);
    } while (rc < 0 && errno == EINTR);
	
	/* 释放锁 */
	munmap( pl, sizeof(msd_lock_t) );
}

#elif defined(MSD_FCNTL_LOCK_MODE)

#ifndef MAXPATHLEN
#   ifdef PAHT_MAX
#       define MAXPATHLEN PATH_MAX
#   elif defined(_POSIX_PATH_MAX)
#       define MAXPATHLEN _POSIX_PATH_MAX
#   else
#       define MAXPATHLEN   256
#   endif
#endif

static int strxcat(char *dst, const char *src, int size) 
{
    int dst_len = strlen(dst);
    int src_len = strlen(src);
    if (dst_len + src_len < size) 
    {
        memcpy(dst + dst_len, src, src_len + 1);/* 加1，需要复制src末尾的'\0' */
        return MSD_OK;
    } 
    else 
    {
        memcpy(dst + dst_len, src, (size - 1) - dst_len);
        dst[size - 1] = '\0';
        return MSD_ERR;
    }
}

/**
 * 功能: Lock implemention based on 'fcntl'. 
 * 参数: @
 * 描述:
 *      1. mkstemp函数在系统中以唯一的文件名创建一个文件并打开，只有一个参数，
 *         这个参数是个以“XXXXXX”结尾的非空字符串。mkstemp函数会用随机产生的字
 *         符串替换“XXXXXX”，保证了文件名的唯一性。
 *      2. 临时文件使用完成后应及时删除，否则临时文件目录会塞满垃圾。mkstemp函数
 *         创建的临时文件不能自动删除，所以执行完 mkstemp函数后要调用unlink函数，
 *         unlink函数删除文件的目录入口，但临时文件还可以通过文件描述符进行访问，
 *         直到最后一个打开的进程关 闭文件操作符，或者程序退出后临时文件被自动彻底地删除。
 * 返回: 成功， 失败，
 **/
int msd_fcntl_init(msd_lock_t **ppl, const char *pathname) 
{
    char s[MAXPATHLEN];
    int fd;

    /* 共享内存之上，生成lock。 */
    *ppl = mmap(0, sizeof(msd_lock_t), PROT_WRITE | PROT_READ,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (*ppl == MAP_FAILED)
    {
        fprintf(stderr, "mmap lock failed: %s\n",strerror(errno));
        return MSD_ERR;
    }
    
    strncpy(s, pathname, MAXPATHLEN - 1);
    strxcat(s, ".sem.XXXXXX", MAXPATHLEN);    
    
    /* 生成唯一的文件，并打开 */
    fd = mkstemp(s);
    if (fd < 0) 
    {
        return MSD_ERR;
    }
    
    /* 去除引用 */
    unlink(s);

    (*ppl)->fd = fd;
    //printf("Thread lock init\n");
    return fd;
}
/**
 * 功能: lock
 * 参数: @fd
 *       @flag, 读锁或写锁
 * 返回: 成功， 失败，
 **/
int msd_fcntl_lock(int fd, int flag) 
{
    int rc;
    struct flock l;

    /*整个文件锁定*/
    l.l_whence  = SEEK_SET;
    l.l_start   = 0;
    l.l_len     = 0;
    l.l_pid     = 0;

    if (flag == MSD_LOCK_RD) 
    {
        l.l_type = F_RDLCK; /* 建立读取锁 */
    } 
    else 
    {
        l.l_type = F_WRLCK; /* 建立读写锁 */
    }
    
    /*F_SETLKW 和F_SETLK 作用相同，但是无法建立锁定时，此调用会一直等到锁定动作成功为止。*/
    rc = fcntl(fd, F_SETLKW, &l);
    return rc;
}

/**
 * 功能: unlock
 * 参数: @fd
 * 返回: 成功， 失败，
 **/
int msd_fcntl_unlock(int fd) 
{
    int rc;
    struct flock l;
    l.l_whence = SEEK_SET;
    l.l_start = 0;
    l.l_len = 0;
    l.l_pid = 0;
    l.l_type = F_UNLCK;/* 解锁（删除锁） */

    rc = fcntl(fd, F_SETLKW, &l);
    return rc;
}

/**
 * 功能: destroy lock
 * 参数: @pl @fd
 * 返回: 成功， 失败，
 **/
void msd_fcntl_destroy(msd_lock_t *pl, int fd) 
{
    close(fd);

    /* 释放锁 */
	munmap( pl, sizeof(msd_lock_t) );
}
#endif

#ifdef __MSD_LOCK_TEST_MAIN__

static msd_lock_t *msd_lock;/* 锁 */

void call_lock()
{
    /*
        若锁无效，整个程序执行时间大概为5秒(两个进程并行)
        否则，整个程序执行时间大概为10秒(锁生效了，锁之间的代码成了临界区)
    */
    MSD_LOCK_LOCK(msd_lock);
    printf("child call lock %d\n", getpid());
    sleep(5);
    MSD_LOCK_UNLOCK(msd_lock);
}

/*测试锁*/
void msd_test_lock()
{
    int i;
    for(i=0; i<2; i++)
    {
        pid_t pid;
        if((pid = fork()) == 0)
        {
            if(getpid()%2)
            {
                printf("child odd %d\n", getpid());
                call_lock();
            }
            else
            {
                printf("child even %d\n", getpid());
                call_lock();
            }
            exit(0);
        }
        else
        {
            printf("spawn child %d\n", pid);
        }
    }
    //父进程等待子进程退出
    int status;
    for(i=0; i<2; i++)
    {
        waitpid( -1, &status, 0 );
    }    
}

void call_lock_in_pthread(void *arg)
{
    msd_lock_t *lock = (msd_lock_t *)arg;
    MSD_LOCK_LOCK(lock);
    printf("child call lock %ul\n", (unsigned long )pthread_self());
    sleep(5);
    MSD_LOCK_UNLOCK(lock);
}

void msd_test_lock_in_pthread()
{
    int i;
    pthread_t thread[2];               /*保存线程号*/

    for(i=0; i<2; i++)
    {
        pthread_create(&thread[i], NULL, (void *)call_lock_in_pthread, msd_lock);
        printf("create thread %ul\n", (unsigned long )thread[i]);
    }    
    //join
    for(i=0; i<2; i++)
    {
        pthread_join(thread[i], NULL);
    }   
    printf("Master thread exit!\n");
}
 
int main()
{
    if (MSD_LOCK_INIT(msd_lock) != 0) 
    {
        return MSD_ERR;
    }
    
    /* 测试锁在多进程中的效果 */
    //msd_test_lock();

    /* 测试锁在多线程中的效果 */
    msd_test_lock_in_pthread();
    
    MSD_LOCK_DESTROY(msd_lock);
    return MSD_OK;
}
#endif
