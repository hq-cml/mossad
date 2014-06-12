/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_dlist.c
 * 
 * Description :  Msd_dlist, a generic doubly linked list implementation.
 * 
 *     Created :  Apr 4, 2012 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/
#include "msd_core.h"
 
/**
 * 功能: Initialize a new list.
 * 参数: @
 * 描述:
 *      1. The initalized list should be freed with dlist_destroy(), 
 *         but private value of each node need to be freed by the 
 *         user before to call dlist_destroy if list's 'free' pointer
 *         is NULL.
 * 返回: 成功，dlist结构指针， 失败，NULL
 **/
msd_dlist_t *msd_dlist_init(void) 
{
    msd_dlist_t *dl;

    if ((dl = malloc(sizeof(*dl))) == NULL) 
    {
        return NULL;
    }

    dl->head = NULL;
    dl->tail = NULL;
    dl->len = 0;
    dl->dup = NULL;
    dl->free = NULL;
    dl->match = NULL;
    return dl;
}
/**
 * 功能: Destroy the whole list. 
 * 参数: @dl，dlist结构地址
 * 描述:
 *      1. If the list's 'free' pointer is not NULL, 
 *         the value will be freed automately first.
 **/
void msd_dlist_destroy(msd_dlist_t *dl) 
{
    unsigned int len;
    msd_dlist_node_t *curr = NULL;
    msd_dlist_node_t *next = NULL;

    curr = dl->head;
    len = dl->len;
    while (len--) 
    {
        next = curr->next;
        if (dl->free)
            dl->free(curr->value);
        free(curr);
        curr = next;
    }
    free(dl);
}

/**
 * 功能: Add a new node to the list's head, containing the specified 
 *       'value' pointer as value. 
 * 参数: @dl, dlist结构指针
 *       @value, value指针
 * 描述:
 *      1. 头部插入一个节点（无头结点）
 *      2. 返回的值是传入时候的指针
 * 返回: 成功，dlist结构指针,失败，NULL
 **/
msd_dlist_t *msd_dlist_add_node_head(msd_dlist_t *dl, void *value) 
{
    msd_dlist_node_t *node = NULL;
    if ((node = malloc(sizeof(*node))) == NULL) 
    {
        return NULL;
    }
    
    if(dl->dup)
        node->value = dl->dup(value);
    else
        node->value = value;

    if (dl->len == 0) 
    {
        /* 第一次 */
        dl->head = node;
        dl->tail = node;
        node->prev = NULL;
        node->next = NULL;
    } 
    else 
    {
        node->prev = NULL;
        node->next = dl->head;
        dl->head->prev = node;
        dl->head = node;
    }
    dl->len++;
    return dl;
}
 
/**
 * 功能: Add a new node to the list's tail, containing the specified
 *       'value' pointer as value.
 * 参数: @
 * 描述:
 *      1. 
 * 返回: 成功，dlist结构指针,失败，NULL
 **/
msd_dlist_t *msd_dlist_add_node_tail(msd_dlist_t *dl, void *value) 
{
    msd_dlist_node_t *node;
    if ((node = malloc(sizeof(*node))) == NULL) 
    {
        return NULL;
    }

    if(dl->dup)
        node->value = dl->dup(value);
    else
        node->value = value;

    if (dl->len == 0) 
    {
        /* 第一次 */
        dl->head = node;
        dl->tail = node;
        node->prev = NULL;
        node->next = NULL;
    } 
    else 
    {
        node->prev = dl->tail;
        node->next = NULL;
        dl->tail->next = node;
        dl->tail = node;
    }
    dl->len++;
    return dl;
}

/**
 * 功能: Add a new node to the list's midle, 
 * 参数: @dl @mid_node @value @after
 * 描述:
 *      1. mid_node插入基准点，after==1，则在基准之后插入
 *         after==0 在基准点之前插入
 * 返回: 成功 ， 失败，
 **/
msd_dlist_t *msd_dlist_insert_node(msd_dlist_t *dl, msd_dlist_node_t *mid_node, 
        void *value, int after) 
{
    msd_dlist_node_t *node;
    if ((node = malloc(sizeof(*node))) == NULL)
        return NULL;

    node->value = value;
    if (after) 
    {
        /*基点之后插入*/
        node->prev = mid_node;
        node->next = mid_node->next;
        if (dl->tail == mid_node) 
        {
            dl->tail = node;
        }
    } 
    else 
    {
        /*基点之前插入*/
        node->next = mid_node;
        node->prev = mid_node->prev;
        if (dl->head == mid_node) 
        {
            dl->head = node;
        }
    }

    if (node->prev != NULL) 
    {
        node->prev->next = node;
    }

    if (node->next != NULL) 
    {
        node->next->prev = node;
    }
    dl->len++;
    return dl;
}

/**
 * 功能: Remove the specified node from the specified list.
 * 参数: @dl @node
 * 描述:
 *      1. If the list's 'free' pointer is not NULL, 
 *         the value will be freed automately first.
 **/
void msd_dlist_delete_node(msd_dlist_t *dl, msd_dlist_node_t *node) 
{
    if (node->prev) 
    {
        node->prev->next = node->next;
    } 
    else 
    {
        dl->head = node->next;
    }

    if (node->next) 
    {
        node->next->prev = node->prev;
    } 
    else 
    {
        dl->tail = node->prev;
    }

    if (dl->free) 
    {
        dl->free(node->value);
    }

    free(node);
    dl->len--;
}

/**
 * 功能: create a list iterator.
 * 参数: @dl
 *       @direction，方向：头->尾/尾->头
 * 描述:
 *      1. 
 * 返回: 成功，iter结构指针，失败，NULL
 **/
msd_dlist_iter_t *msd_dlist_get_iterator(msd_dlist_t *dl, int direction) 
{
    msd_dlist_iter_t *iter;
    if ((iter = malloc(sizeof(*iter))) == NULL)
        return NULL;

    if (direction == MSD_DLIST_START_HEAD) 
        iter->node= dl->head; 
    else
        iter->node = dl->tail;
    
    iter->direction = direction;
    return iter;
}

/**
 * 功能: Release the iterator memory.
 **/
void msd_dlist_destroy_iterator(msd_dlist_iter_t *iter) 
{
    if (iter) 
        free(iter);
}

/**
 * 功能: Create an iterator in the list iterator structure.
 **/
void msd_dlist_rewind(msd_dlist_t *dl, msd_dlist_iter_t *iter) 
{
    iter->node = dl->head;
    iter->direction = MSD_DLIST_START_HEAD;
}

/**
 * 功能: Create an iterator in the list iterator structure.
 **/
void msd_dlist_rewind_tail(msd_dlist_t *dl, msd_dlist_iter_t *iter) 
{
    iter->node = dl->tail;
    iter->direction = MSD_DLIST_START_TAIL;
}

/**
 * 功能: 按照指针的方向，移动指针.
 * 参数: @iter
 * 描述:
 *      1. 返回值是指针初始指向的元素
 *      2. the classical usage patter is:
 *      iter = msd_dlist_get_iterator(dl, <direction>);
 *      while ((node = msd_dlist_next(iter)) != NULL) {
 *          do_something(dlist_node_value(node));
 *      }
 * 返回: 成功，node结构指针，失败，NULL
 **/
msd_dlist_node_t *msd_dlist_next(msd_dlist_iter_t *iter) 
{
    msd_dlist_node_t *curr = iter->node;
    if (curr != NULL) 
    {
        if (iter->direction == MSD_DLIST_START_HEAD)
            iter->node = curr->next;
        else 
            iter->node = curr->prev;/* PREPARE TO DELETE ? */
    }
    return curr;
}

/**
 * 功能: Search the list for a node matching a given key.
 * 参数: @dl @key
 * 描述:
 *      1. 如果match函数存在，则用match函数比较，否则直接比较value指针和key
 * 返回: 成功，node指针,失败，NULL
 **/
msd_dlist_node_t *msd_dlist_search_key(msd_dlist_t *dl, void *key) 
{
    msd_dlist_iter_t *iter = NULL;
    msd_dlist_node_t *node = NULL;
    iter = msd_dlist_get_iterator(dl, MSD_DLIST_START_HEAD);
    while ((node = msd_dlist_next(iter)) != NULL) 
    {
        if (dl->match) 
        {
            if (dl->match(node->value, key)) 
            {
                msd_dlist_destroy_iterator(iter);
                return node;
            }
        } 
        else 
        {
            if (key == node->value) 
            {
                msd_dlist_destroy_iterator(iter);
                return node;
            }
        }
    }
    msd_dlist_destroy_iterator(iter);
    return NULL;
}

/**
 * 功能: Duplicate the whole list. 
 * 参数: @orig
 * 描述:
 *      1. The 'dup' method set with dlist_set_dup() function is used to copy the
 *         node value.Other wise the same pointer value of the original node is 
 *         used as value of the copied node. 
 * 返回: 成功，新的dlist结构指针，失败，NULL
 **/
msd_dlist_t *msd_dlist_dup(msd_dlist_t *orig) 
{
    msd_dlist_t *copy;
    msd_dlist_iter_t *iter;
    msd_dlist_node_t *node;

    if ((copy = msd_dlist_init()) == NULL) 
    {
        return NULL;
    }

    copy->dup = orig->dup;
    copy->free = orig->free;
    copy->match = orig->match;
    iter = msd_dlist_get_iterator(orig, MSD_DLIST_START_HEAD);
    while ((node = msd_dlist_next(iter)) != NULL) 
    {
        void *value;
        if (copy->dup) 
        {
            value = copy->dup(node->value);
            if (value == NULL) 
            {
                msd_dlist_destroy(copy);
                msd_dlist_destroy_iterator(iter);
                return NULL;
            }
        } 
        else 
        {
            value = node->value;
        }
        if (msd_dlist_add_node_tail(copy, value) == NULL) 
        {
            msd_dlist_destroy(copy);
            msd_dlist_destroy_iterator(iter);
            return NULL;
        }
    } 
    msd_dlist_destroy_iterator(iter);
    return copy;
}

/**
 * 功能: 从头或尾，向后或向前找到第index个元素，返回指针 
 * 参数: @
 * 描述:
 *      1. index where 0 is the head, 1 is the element next
 *         to head and so on. Negative integers are used in 
 *         order to count from the tail, -1 is the last element,
 *         -2 the penultimante and so on. 
 * 返回: 成功，node指针,失败，NULL
 **/
msd_dlist_node_t *msd_dlist_index(msd_dlist_t *dl, int index) 
{
    msd_dlist_node_t *node;

    if (index < 0) 
    {
        /* index小于0，则从尾部向前找 */
        index = (-index) - 1;
        node = dl->tail;
        while (index-- && node) 
        {
            node = node->prev;
        }
    } 
    else 
    {
        node = dl->head;
        while (index-- && node) 
        {
            node = node->next;
        }
    }
    return node;
}

#ifdef __MSD_DLIST_TEST_MAIN__
#include <stdio.h>
int main()
{
    msd_dlist_t *dl;
    msd_dlist_t *copy;
    msd_dlist_node_t *node;
    
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;
    
    if ((dl = msd_dlist_init()) == NULL)
    {
        printf("init failed!\n");
        return 0;
    }
/*
    msd_dlist_add_node_head(dl, &a);
    msd_dlist_add_node_head(dl, &b);
    msd_dlist_add_node_head(dl, &c);
    msd_dlist_add_node_head(dl, &d);
*/
/*
    msd_dlist_add_node_tail(dl, &a);
    msd_dlist_add_node_tail(dl, &b);
    msd_dlist_add_node_tail(dl, &c);
    msd_dlist_add_node_tail(dl, &d);
*/
/* 
    msd_dlist_add_node_tail(dl, &a);
    msd_dlist_add_node_tail(dl, &b);
    msd_dlist_add_node_tail(dl, &c);
    node = msd_dlist_search_key(dl, &b);
    msd_dlist_insert_node(dl, node, &d, 1);
*/ 
    msd_dlist_add_node_tail(dl, &a);
    msd_dlist_add_node_tail(dl, &b);
    msd_dlist_add_node_tail(dl, &c);
    node = msd_dlist_search_key(dl, &c);
    msd_dlist_insert_node(dl, node, &d, 0);
    
    node = dl->head;
    while(node)
    {
        printf("%d\n",*((int *)node->value));
        node = node->next;
    }

    copy = msd_dlist_dup(dl);

    node = copy->head;
    while(node)
    {
        printf("%d\n",*((int *)node->value));
        node = node->next;
    }

    msd_dlist_destroy(dl);
    msd_dlist_destroy(copy);
    return 0;
}

#endif /* __MSD_DLIST_TEST_MAIN__ */

