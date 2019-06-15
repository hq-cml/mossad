/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_hash.h 
 * 
 * Description :  Msd_hash, a simple hash library. 
 *                Maybe it's not complex than redis's dict, but is enough.
 * 
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/

#ifndef __MSD_HASH_H_INCLUDED__
#define __MSD_HASH_H_INCLUDED__
 
/*
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <time.h>

#define MSD_OK       0
#define MSD_ERR     -1
#define MSD_FAILED  -2
#define MSD_NONEED  -3
#define MSD_END     -4
*/

#define MSD_HASH_INIT_SLOTS 16
#define MSD_HASH_RESIZE_RATIO 1 /* (ht->count / ht->slots) */

/* assign macros */
#define MSD_HASH_SET_KEYCMP(ht, func)   (ht)->keycmp   = (func)
#define MSD_HASH_SET_SET_KEY(ht, func)  (ht)->set_key  = (func)
#define MSD_HASH_SET_SET_VAL(ht, func)  (ht)->set_val  = (func)
#define MSD_HASH_SET_FREE_KEY(ht, func) (ht)->free_key = (func)
#define MSD_HASH_SET_FREE_VAL(ht, func) (ht)->free_val = (func)
    
#define MSD_HASH_SET_KEY(ht, entry, _key_) do {\
    if((ht)->set_key)                           \
        entry->key = (ht)->set_key(_key_);      \
    else                                       \
        entry->key = (_key_);                  \
}while(0)

#define MSD_HASH_SET_VAL(ht, entry, _val_) do {\
    if((ht)->set_val)                           \
        entry->val = (ht)->set_val(_val_);      \
    else                                       \
        entry->val = (_val_);                  \
}while(0)

#define MSD_HASH_FREE_KEY(ht, entry) do {\
    if ((ht)->free_key)                  \
        (ht)->free_key((entry)->key);    \
}while(0)

#define MSD_HASH_FREE_VAL(ht, entry) do {\
    if ((ht)->free_val)                  \
        (ht)->free_val((entry)->val);    \
}while(0)

#define MSD_HASH_CMP_KEYS(ht, key1, key2)   \
    ((ht)->keycmp) ?                        \
    (ht)->keycmp(key1, key2):(key1)==(key2)

typedef struct _hash_entry_t
{
    void *key;
    void *val;
    struct _hash_entry_t *next;
}msd_hash_entry_t;

typedef struct _hash_t
{
    unsigned int     slots;  /* the num of slots */
    unsigned int     count;  /* the total num of key/val pairs */
    msd_hash_entry_t **data; /* the table */

    /* call back:implemantation associated function pointers */
    int   (*keycmp)(const void *, const void *);
    void *(*set_key)(const void *);
    void *(*set_val)(const void *);
    void  (*free_key)(void *);/* free_key函数保证不发生内存泄露 */
    void  (*free_val)(void *);
}msd_hash_t;

typedef struct _hash_iter_t
{
    int                 pos;   /*在data数组中的位置*/
    int                 depth; /*在data某元素的链表中的位置*/

    msd_hash_entry_t    *he;
    msd_hash_t          *ht;
    void                *key;
    void                *val;
}msd_hash_iter_t;

msd_hash_t *msd_hash_create(unsigned int slots);
int msd_hash_insert(msd_hash_t *ht, const void *key, const void *val);
int msd_hash_remove_entry(msd_hash_t *ht, const void *key);
int msd_hash_foreach(msd_hash_t *ht,
        int (*foreach)(const msd_hash_entry_t *he, void *userptr),
        void *userptr);
void *msd_hash_get_val(msd_hash_t *ht, const void *key);
void msd_hash_destroy(msd_hash_t *ht);
void msd_hash_clear(msd_hash_t *ht);
void msd_hash_free(msd_hash_t *ht);
msd_hash_t *msd_hash_duplicate(msd_hash_t *ht);
msd_hash_iter_t *msd_hash_iter_new(msd_hash_t *ht);
int msd_hash_iter_move_next(msd_hash_iter_t *iter);
int msd_hash_iter_move_prev(msd_hash_iter_t *iter);
int msd_hash_iter_reset(msd_hash_iter_t *iter);
void msd_hash_iter_free(msd_hash_iter_t *iter);
void *msd_hash_def_set(const void *key);
void msd_hash_def_free(void *key);
int msd_hash_def_cmp(const void *key1, const void *key2);

#endif /* __MSD_HASH_H_INCLUDED__ */

