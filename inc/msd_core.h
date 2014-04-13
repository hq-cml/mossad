/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *     Filename:  Msd_core.h 
 * 
 *  Description:  The massod's public header file. 
 * 
 *      Created:  Mar 18, 2012 
 *      Version:  0.0.1 
 * 
 *       Author:  HQ 
 *      Company:  Qihoo 
 *
 **/

#ifndef __MSD_CORE_H_INCLUDED__ 
#define __MSD_CORE_H_INCLUDED__

/* --------SYSTEM HEADER FILE---------- */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <strings.h>
#include <libgen.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <getopt.h>

/* ---------CONFIG--------- */
#define MSD_PTHREAD_LOCK_MODE       /* Lock mode */
//#define MSD_SYSVSEM_LOCK_MODE
//#define MSD_FCNTL_LOCK_MODE

#define MSD_LOG_MODE_THREAD         /* Log mode */
//#define MSD_LOG_MODE_PROCESS

#define MSD_EPOLL_MODE              /* Multiplexing mode */
//#define MSD_SELECT_MODE


/* ----PROJECT HEADER FILE---- */
#include "msd_lock.h"
#include "msd_log.h"
#include "msd_string.h"
#include "msd_hash.h"
#include "msd_conf.h"
#include "msd_vector.h"
#include "msd_dlist.h"
#include "msd_ae.h"

/* -------PUBLIC MACRO------- */
#define MSD_OK       0
#define MSD_ERR     -1
#define MSD_FAILED  -2
#define MSD_NONEED  -3
#define MSD_END     -4


#define MSD_PROG_TITLE   "Mossad"
#define MSD_PROG_NAME    "mossad"
#define MOSSAD_VERSION   "0.0.1"

/* --------Structure--------- */
typedef struct _msd_instance_t{

    msd_str_t   *conf_path;
    msd_conf_t  *conf;
    msd_log_t   *log;
}msd_instance_t;
















#endif
