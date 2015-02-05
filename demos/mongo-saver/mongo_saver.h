/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  mongo_saver.h
 * 
 * Description :  mongo_saver
 * 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/
#ifndef __MONGO__
#define __MONGO__
#include "msd_core.h"
#include "cJSON.h"
#include <curl/curl.h>
#include <float.h>
#include <limits.h>

#include <bson.h>
#include <mongoc.h>
  
/* worker私有数据 */
typedef struct saver_worker_data{ 
    msd_str_t           *mongo_ip; 
    msd_str_t           *mongo_port;
    msd_str_t           *mongo_db; 
    msd_str_t           *mongo_table;
    msd_thread_worker_t *worker;              /* worker_data所依附的worker句柄 */
}saver_worker_data_t;

#endif
