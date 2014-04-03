/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_conf.h 
 * 
 * Description :  Msd_conf, a configure library. Support include and block.
 * 
 *     Created :  Mar 31, 2014 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/


/*
 *  一个配置文件读取的库，能够支持include和block。如果出现相同的key,则会存在一个链表里面
 */
#ifndef __MSD_CONF_H_INCLUDED__
#define __MSD_CONF_H_INCLUDED__

/*
#include <stdio.h>
#include <stdlib.h> 
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>

#include "msd_string.h"
#include "msd_hash.h"

#define MSD_OK       0
#define MSD_ERR     -1
#define MSD_FAILED  -2
*/

#define MSD_CONF_TYPE_ENTRY    1
#define MSD_CONF_TYPE_BLOCK    2

#define MAX_LINE               1023

typedef struct msd_conf_value
{
    void          *value;
    unsigned char type;
}msd_conf_value_t;

typedef struct msd_conf_t
{
    msd_hash_t  *ht;
}msd_conf_t;

/* 普通配置条目 */
typedef struct msd_conf_entry
{
    char                  *value;
    struct msd_conf_entry *next;
}msd_conf_entry_t;

/* block:{}内部的内容 */
typedef struct msd_conf_block
{
    msd_conf_t             block;
    struct msd_conf_block  *next; /* 递归 */
}msd_conf_block_t;

int msd_conf_init(msd_conf_t *conf, const char *filename);
int msd_conf_get_int_value(msd_conf_t *conf, const char *key, int def);
char *msd_conf_get_str_value(msd_conf_t *conf, const char *key, char *def);
msd_conf_block_t *msd_conf_get_block(msd_conf_t *conf, char *key);
void msd_conf_free(msd_conf_t *conf);
void msd_conf_dump(msd_conf_t *conf);
#endif /* __MSD_CONF_H_INCLUDED__ */
