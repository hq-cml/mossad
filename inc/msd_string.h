/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_string.h 
 * 
 * Description :  Msd_string, a C dynamic strings library.
 *                Derived from redis.
 *
 *        注意 :  所有内部调用msd_str_expand_room的函数，返回的msd_str_t *可能
 *                是一个新地址，所以需要用链式表达，如下：
 *                g_instance->conf_path = msd_str_cpy(g_instance->conf_path,
 *                                                    "./mossad.conf");
 * 
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/

#ifndef __MSD_STRING_H_INCLUDED__ 
#define __MSD_STRING_H_INCLUDED__
 
/*
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
*/
#define MSD_STRING_MAX_PREALLOC (10*1024*1024)

typedef struct _msd_string_s
{
    int  len;    /* 实际字符串长度,不包括'\0' */
    int  free;   /* 空闲长度,不包括'\0' */
                 /* 综上，所以buf的总空间=len+free+1 */
    char buf[];  /* 数据开始指针 */
}msd_str_t;

msd_str_t* msd_str_new(const void *init);
msd_str_t* msd_str_new_empty(void);
msd_str_t* msd_str_dup(msd_str_t *init);
void       msd_str_free(msd_str_t *pstr);
void       msd_str_clear(msd_str_t *pstr);
void       msd_str_incr_len(msd_str_t *pstr, int incr);
msd_str_t* msd_str_cat_len(msd_str_t **pstr, const void *t, size_t len);
msd_str_t* msd_str_cat(msd_str_t **pstr, const void *t);
msd_str_t* msd_str_cat_msd_str(msd_str_t **pstr, msd_str_t *pstr1);
msd_str_t* msd_str_cpy_len(msd_str_t **pstr, const void *t, size_t len);
msd_str_t* msd_str_cpy(msd_str_t **pstr, const void *t);
msd_str_t* msd_str_cat_sprintf(msd_str_t **pstr, const char *fmt, ...);
msd_str_t* msd_str_sprintf(msd_str_t **pstr, const char *fmt, ...);
void       msd_str_trim(msd_str_t *pstr, const char *cset);
int        msd_str_range(msd_str_t *pstr, int start, int end);
void       msd_str_tolower(char *s);
void       msd_str_toupper(char *s);
int        msd_str_cmp(const char *s1, const char *s2);
int        msd_str_explode(unsigned char *buf, unsigned char *field[], \
           int n, const unsigned char *ifs);
#endif

