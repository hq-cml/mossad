/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_vector.h
 * 
 * Description :  Msd_vector, a vector based on array. Support any type.
 *                在一整片(size*slots)连续的内存中，模拟出数组的行为。
 * 
 *     Created :  Apr 4, 2012
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/

#ifndef __MSD_VECTOR_H_INCLUDED__
#define __MSD_VECTOR_H_INCLUDED__

/*
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MSD_OK       0
#define MSD_ERR     -1
*/

typedef struct msd_vector
{
    unsigned int  count; /* vector中元素个数，Vector的索引从0开始，即count<=slots-1 
                          * 注意!这个值是有问题的!Vector不能确认准确的元素个数!
                          */
    unsigned int  slots; /* data数组槽位数量，即可容纳元素的最大个数 */
    unsigned int  size;  /* the size of a member，每个元素的大小 */
    void         *data;  /* 元素数组 */
}msd_vector_t;

typedef struct msd_vector_iter
{
    unsigned int  pos;  /* 在数组中的索引，整型 */
    msd_vector_t *vec;  /* 所依附的vector */
    void         *data; /* pos所对应的元素的起始地址 */
}msd_vector_iter_t;


msd_vector_t *msd_vector_new(unsigned int slots, unsigned int size);

int msd_vector_push(msd_vector_t *v, void *data);

int msd_vector_set_at(msd_vector_t *v, unsigned int index, void *data);

void *msd_vector_get_at(msd_vector_t *v, unsigned int index);

void msd_vector_destroy(msd_vector_t *v);

void msd_vector_free(msd_vector_t *v);
 
msd_vector_iter_t *msd_vector_iter_new(msd_vector_t *vec);

int msd_vector_iter_next(msd_vector_iter_t *iter);

int msd_vector_iter_prev(msd_vector_iter_t *iter);

void msd_vector_iter_reset(msd_vector_iter_t *iter);

void msd_vector_iter_destroy(msd_vector_iter_t *iter);

void msd_vector_iter_free(msd_vector_iter_t *iter);

#endif /* __MSD_VECTOR_H_INCLUDED__ */

