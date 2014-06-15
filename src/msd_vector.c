/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_vector.c
 * 
 * Description :  Msd_vector, a vector based on array. Support any type.
 *                在一整片(size*slots)连续的内存中，模拟出数组的行为。
 * 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/

#include "msd_core.h"

/**
 * 功能: init vector
 * 参数: @vec, @slots, @size
 * 描述:
 *      1. 在一整片连续的内存中，模拟出数组的行为
 * 返回: 成功 0， 失败，-x
 **/ 
static int msd_vector_init(msd_vector_t *vec, unsigned int slots, unsigned int size) 
{
    if (!slots)
    {
        slots = 16;
    }    
    vec->slots = slots;
    vec->size = size;
    vec->count = 0;
    
    vec->data = calloc(1, vec->slots * vec->size);    
    if (!vec->data) 
    {
        return MSD_ERR;
    }
    
    return MSD_OK;
}

/**
 * 功能: Create a vector, and init it
 * 参数: @slots, vector元素个数
 *       @size, 每个元素的大小
 * 描述:
 *      1. 
 * 返回: 成功，vector结构地址，失败，NULL
 **/
msd_vector_t *msd_vector_new(unsigned int slots, unsigned int size) 
{
    msd_vector_t *vec = (msd_vector_t*)calloc(1, sizeof(*vec));
    if (!vec) 
        return NULL;
        
    if (msd_vector_init(vec, slots, size) < 0) 
    {
        free(vec);
        return NULL;
    }
    
    return vec;
}

/**
 * 功能: double the number of the slots available to a vector
 * 参数: @vec
 * 描述:
 *      1. realloc，不需要释放原来的地址
 * 返回: 成功 0 失败 -x
 **/ 
static int msd_vector_resize(msd_vector_t *vec) 
{
    void *temp;
    temp = realloc(vec->data, vec->slots * 2 * vec->size);
                
    if (!temp) 
    {
        return MSD_ERR;
    }
    
    vec->data = temp;
    vec->slots *= 2;
    return MSD_OK;
}

/**
 * 功能: vector set at
 * 参数: @vec, @index, @data
 * 注意:
 *       本函数对count的处理时存在问题的，set_at有可能是在清空!
 * 返回: 成功 0 失败 -x
 **/
int msd_vector_set_at(msd_vector_t *vec, unsigned int index, void *data) 
{
    while (index >= vec->slots)
    {
        //TODO vector shoud have a MAX_LENGTH
        /* resize until our table is big enough. */
        if (msd_vector_resize(vec) != MSD_OK) 
        {
            return MSD_ERR;
        }
    }
    
    memcpy((char *)vec->data + (index * vec->size), data, vec->size);
    ++vec->count; /* 错误! */
    return MSD_OK;
} 
 
/**
 * 功能: 在vector尾部添加一个元素
 * 参数: @
 *       @
 * 描述:
 *      1. vector索引从0开始
 *      2. 这个函数不能用!，因为它依赖count!
 * 返回: 成功 0 失败 -x
 **/  
int msd_vector_push(msd_vector_t *vec, void *data) 
{
    if (vec->count == (vec->slots - 1)) 
    {
        if (msd_vector_resize(vec) != MSD_OK) 
        {
            return MSD_ERR;
        }
    }
    
    return msd_vector_set_at(vec, vec->count, data);
}

 /**
 * 功能: get a value random
 * 参数: @vec , @index
 **/
void *msd_vector_get_at(msd_vector_t *vec, unsigned int index) 
{
    if (index >= vec->slots) 
        return NULL;

    return (char *)vec->data + (index * vec->size);
}

/**
 * 功能: destroy the vector
 * 参数: @vec
 * 描述:
 *      1. 仅释放data，vec本身结构不释放
 **/
void msd_vector_destroy(msd_vector_t *vec) 
{
    if (vec->data) 
        free(vec->data);
    
    vec->slots = 0;
    vec->count = 0;
    vec->size = 0;
    vec->data = NULL;
}

/**
 * 功能: destroy the vector
 * 参数: @vec
 * 描述:
 *      1. 释放data，然后释放vec本身
 **/
void msd_vector_free(msd_vector_t *vec) 
{
    msd_vector_destroy(vec);
    free(vec);
}
 
/**
 * 功能: init vector
 * 参数: @iter
 *       @vec
 * 描述:
 *      1. 初始化完成后，将iter指向起始元素
 * 返回: 成功 0 失败 -x
 **/  
static int msd_vector_iter_init(msd_vector_iter_t *iter, msd_vector_t *vec) 
{
    iter->pos = 0;
    iter->vec = vec;
    iter->data = NULL;
    /* 指向起始位置 */
    return msd_vector_iter_next(iter);
}

/**
 * 功能: new vector
 * 参数: @vec
 * 返回: 成功 iter指针 失败 NULL
 **/ 
msd_vector_iter_t *msd_vector_iter_new(msd_vector_t *vec) 
{
    msd_vector_iter_t *iter;
    
    iter = (msd_vector_iter_t *)calloc(1, sizeof(*iter));
    if (!iter) 
    {
        return NULL;
    }
    
    if (msd_vector_iter_init(iter, vec) != MSD_OK) 
    {
        free(iter);
        return NULL;
    }
    
    return iter;
}

/**
 * 功能: iter move next
 * 参数: @iter
 * 描述:
 *      1. 指针后移，pos自增，data移动到后一个元素的开始位置
 *      2. 这个函数不能用!，因为它依赖count!
 * 返回: 成功 0 失败 -x
 **/
int msd_vector_iter_next(msd_vector_iter_t *iter) 
{        
    if (iter->pos == (iter->vec->count - 1)) 
    {
        /* 达到末尾 */
        return MSD_ERR;
    }

    if (!iter->data && !iter->pos) 
    {
        /* run for the first time */
        iter->data = iter->vec->data;
        return MSD_OK;
    } 
    else 
    {
        ++iter->pos;
        iter->data = (char *)iter->vec->data + (iter->pos * iter->vec->size);
        return MSD_OK;
    }
}

/**
 * 功能: iter move next
 * 参数: @iter
 * 描述:
 *      1. 指针前移，pos自减，data移动到前一个元素的开始位置
 *      2. 这个函数不能用!，因为它依赖count!
 * 返回: 成功 0 失败 -x
 **/
int msd_vector_iter_prev(msd_vector_iter_t *iter) 
{
    if ((!iter->pos) ? 1 : 0) 
    {
        /* 处于开头 */
        return MSD_ERR;
    }
    
    --iter->pos;
    iter->data = (char *)iter->vec->data + (iter->pos * iter->vec->size);
    return 0;
}

/**
 * 功能: destroy iter
 **/
void msd_vector_iter_destroy(msd_vector_iter_t *iter) 
{
    iter->pos = 0;
    iter->data = NULL;
    iter->vec = NULL;
}

/**
 * 功能: free
 **/
void msd_vector_iter_free(msd_vector_iter_t *iter) 
{
    msd_vector_iter_destroy(iter);
    free(iter);
}

/**
 * 功能: free
 **/
void msd_vector_iter_reset(msd_vector_iter_t *iter) 
{
    msd_vector_t *tmp = iter->vec;
    msd_vector_iter_destroy(iter);
    msd_vector_iter_init(iter, tmp);
}
 
#ifdef __MSD_VECTOR_TEST_MAIN__
#define COUNT       100
typedef struct _test{
    int a;
    int b;
}test_t;

int main(int argc, char **argv) 
{
    int i = 0;
    test_t *value;
    test_t t;
    
    msd_vector_t *vec = msd_vector_new(32, sizeof(test_t));
    for (i = 0; i < COUNT; ++i) 
    {
        t.a = i;
        t.b = i+100;
        msd_vector_push(vec, &t);
    }
    
    for (i = 0; i < COUNT; ++i) 
    {
        value = (test_t *)msd_vector_get_at(vec, i);
        printf("idx=%d, value.a=%d, value.b=%d\n",i, value->a, value->b);
    }
    printf("slots:%d\n", vec->slots);
    printf("count:%d\n", vec->count);

    msd_vector_iter_t *iter = msd_vector_iter_new(vec);
    
    do {
        value = (test_t*)(iter->data);
        printf("%p, value.a=%d, value.b=%d \n", value, value->a, value->b);
    } while (msd_vector_iter_next(iter) == 0);
    
    msd_vector_iter_free(iter);
    msd_vector_free(vec);

    return 0;
}
#endif /* _MSD_VECTOR_TEST_MAIN__ */

