/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_lock.h
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
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/
 
#ifndef __MSD_LOCK_H_INCLUDED__
#define __MSD_LOCK_H_INCLUDED__

/*
#define MSD_PTHREAD_LOCK_MODE
//#define MSD_SYSVSEM_LOCK_MODE
//#define MSD_FCNTL_LOCK_MODE

#include <stdio.h>
#include <pthread.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>

#define MSD_OK       0
#define MSD_ERR     -1
*/

#ifdef MSD_PTHREAD_LOCK_MODE
typedef struct msd_lock_struct 
{
    pthread_mutex_t mutex;
} msd_lock_t;

int msd_pthread_lock_init(msd_lock_t **ppl);
int msd_pthread_do_lock(msd_lock_t *pl);
int msd_pthread_do_unlock(msd_lock_t *pl);
void msd_pthread_lock_destroy(msd_lock_t *pl);

#define MSD_LOCK_INIT(l)    \
    ((msd_pthread_lock_init(&l) != 0) ? -1 : 0)
#define MSD_LOCK_LOCK(l)    msd_pthread_do_lock(l)
#define MSD_LOCK_UNLOCK(l)  msd_pthread_do_unlock(l)
#define MSD_LOCK_DESTROY(l) msd_pthread_lock_destroy(l)

#elif defined(MSD_SYSVSEM_LOCK_MODE)
#include <sys/ipc.h>
#include <sys/sem.h>

typedef struct msd_lock_struct 
{
    int semid;
} msd_lock_t;

/* System V semaphore implementation of lock. */
int msd_sysv_sem_init(msd_lock_t **ppl, const char *pathname);
int msd_sysv_sem_lock(int semid);
int msd_sysv_sem_unlock(int semid);
void msd_sysv_sem_destroy(msd_lock_t *pl, int semid);

#define MSD_LOCK_INIT(l) \
    ((msd_sysv_sem_init(&l, NULL) < 0) ? -1 : 0)
#define MSD_LOCK_LOCK(l)    msd_sysv_sem_lock((l)->semid)
#define MSD_LOCK_UNLOCK(l)  msd_sysv_sem_unlock((l)->semid)
#define MSD_LOCK_DESTROY(l) msd_sysv_sem_destroy((l), (l)->semid)

#elif defined(MSD_FCNTL_LOCK_MODE)
#include <sys/file.h>

typedef struct msd_lock_struct 
{
    int fd;
} msd_lock_t;

#define MSD_LOCK_RD 0
#define MSD_LOCK_WR 1
int msd_fcntl_init(msd_lock_t **ppl, const char *pathname);
int msd_fcntl_lock(int fd, int flag);
int msd_fcntl_unlock(int fd);
void msd_fcntl_destroy(msd_lock_t *pl, int fd);

#define MSD_LOCK_INIT(l)    \
    ((( msd_fcntl_init(&l, "/tmp/mossad")) != -1) ? 0 : -1)
#define MSD_LOCK_LOCK(l)    msd_fcntl_lock((l)->fd, MSD_LOCK_WR)
#define MSD_LOCK_UNLOCK(l)  msd_fcntl_unlock((l)->fd)
#define MSD_LOCK_DESTROY(l) msd_fcntl_destroy((l), (l)->fd)

#endif

#endif /* __MSD_LOCK_H_INCLUDED__ */

