/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_dlist.h
 * 
 * Description :  Msd_dlist, a generic doubly linked list implementation.
 * 
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 **/
#ifndef __MSD_DLIST_H_INCLUDED__
#define __MSD_DLIST_H_INCLUDED__
/*
#include <stdio.h>
#include <stdlib.h>
*/
/* Directions for iterators */
#define MSD_DLIST_START_HEAD    0
#define MSD_DLIST_START_TAIL    1

typedef struct msd_dlist_node 
{
    struct msd_dlist_node *prev;
    struct msd_dlist_node *next;
    void *value;
} msd_dlist_node_t;

typedef struct msd_dlist 
{
    unsigned int len;                     /* 元素个数 */
    msd_dlist_node_t *head;               /* 第一个节点 */
    msd_dlist_node_t *tail;               /* 最后一个节点 */
    void *(*dup)(void *ptr);            
    void (*free)(void *ptr);              /* free每个节点 */
    int (*match)(void *ptr, void *key);

} msd_dlist_t;

typedef struct msd_dlist_iter 
{
    msd_dlist_node_t *node; /* 该迭代器对应的元素的地址 */
    int direction;
} msd_dlist_iter_t;

/* Functions implemented as macros */
#define msd_dlist_length(l)     ((l)->len)
#define msd_dlist_first(l)      ((l)->head) 
#define msd_dlist_last(l)       ((l)->tail)
#define msd_dlist_prev_node(n)  ((n)->prev)
#define msd_dlist_next_node(n)  ((n)->next)
#define msd_dlist_node_value(n) ((n)->value)

#define msd_dlist_set_dup(l,m)      ((l)->dup = (m))
#define msd_dlist_set_free(l,m)     ((l)->free = (m))
#define msd_dlist_set_match(l,m)    ((l)->match = (m)) 

#define msd_dlist_get_dup(l)    ((l)->dup)
#define msd_dlist_get_free(l)   ((l)->free)
#define msd_dlist_get_match(l)  ((l)->match)

/* Prototypes */
msd_dlist_t *msd_dlist_init(void);
void msd_dlist_destroy(msd_dlist_t *dl);
msd_dlist_t *msd_dlist_add_node_head(msd_dlist_t *dl, void *value);
msd_dlist_t *msd_dlist_add_node_tail(msd_dlist_t *dl, void *value);
msd_dlist_t *msd_dlist_insert_node(msd_dlist_t *dl, msd_dlist_node_t *ponode, 
        void *value, int after);
void msd_dlist_delete_node(msd_dlist_t *dl, msd_dlist_node_t *pnode);
msd_dlist_iter_t *msd_dlist_get_iterator(msd_dlist_t *dl, int direction);
msd_dlist_node_t *msd_dlist_next(msd_dlist_iter_t *iter);
void msd_dlist_destroy_iterator(msd_dlist_iter_t *iter);
msd_dlist_t *msd_dlist_dup(msd_dlist_t *orig);
msd_dlist_node_t *msd_dlist_search_key(msd_dlist_t *dl, void *key);
msd_dlist_node_t *msd_dlist_index(msd_dlist_t *dl, int index);
 
void msd_dlist_rewind(msd_dlist_t *dl, msd_dlist_iter_t *iter);
void msd_dlist_rewind_tail(msd_dlist_t *dl, msd_dlist_iter_t *iter);
#endif /* __MSD_DLIST_H_INCLUDED__ */

