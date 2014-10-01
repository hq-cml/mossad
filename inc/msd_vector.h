/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
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
 *
 *    Modified :  去除了存在bug的iter
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
    unsigned int  count; /* vector中元素个数，Vector的索引从0开始. count<=slots 
                              * 注意!这个值是有问题的!Vector不能确认准确的元素个数!
                              * 因为set的时候，可能中间出现空洞，如何认定count，要看调用者的理解
                              * 所以最好在外部单独维护vector的元素真实数量，这个地方和unicorn是不同的，因为
                              * unicorn没有set操作。综上，count是Vector是最后一个有效元素后面的索引!!!
                              */
    unsigned int  slots; /* data数组槽位数量，即可容纳元素的最大个数 */
    unsigned int  size;  /* the size of a member，每个元素的大小 */
    void         *data;  /* 元素数组 */
}msd_vector_t;

typedef int (*msd_vector_cmp_t)(const void *, const void *); 
typedef int (*msd_vector_each_t)(void *, void *);

msd_vector_t *msd_vector_new    (unsigned int slots, unsigned int size);
int           msd_vector_push   (msd_vector_t *v, void *elem);
int           msd_vector_set_at (msd_vector_t *v, unsigned int index, void *data);
void         *msd_vector_get_at (msd_vector_t *v, unsigned int index);
void          msd_vector_free     (msd_vector_t *v);
void         *msd_vector_pop     (msd_vector_t *vec);
void         *msd_vector_top      (msd_vector_t *vec); 
int           msd_vector_sort    (msd_vector_t *vec, msd_vector_cmp_t cmp);
int           msd_vector_each   (msd_vector_t *vec, msd_vector_each_t func, void *data);

#endif /* __MSD_VECTOR_H_INCLUDED__ */

