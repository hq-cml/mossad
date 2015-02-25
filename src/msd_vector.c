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
 *     Version :  1.0.0
 * 
 *      Author :  HQ 
 *
 *    Modified :  去除了存在bug的iter
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
        
    if (msd_vector_init(vec, slots, size) != MSD_OK) 
    {
        free(vec);
        return NULL;
    }
    
    return vec;
}

/**
 * 功能: destroy the vector
 * 参数: @vec
 * 描述:
 *      1. 仅释放data，vec本身结构不释放
 **/
static void msd_vector_destroy(msd_vector_t *vec) 
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
 * 功能: 给出一个元素的地址，获得此元素在vector中的索引
 * 参数: @vec, @elem元素地址
 * 返回: 成功，索引，失败，-1
 **/
uint32_t msd_vector_idx(msd_vector_t *vec, void *elem)
{
    uint8_t   *p, *q;    //内存按字节编址
#if __WORDSIZE == 32
    uint32_t   off, idx;
#else
    uint64_t   off, idx;
#endif
    
    if(elem < vec->data){
        return MSD_ERR;
    }

    p = vec->data;
    q = elem;
    
#if __WORDSIZE == 32
    off = (uint32_t)(q - p);
    if(off % (uint32_t)vec->size != 0){
        return MSD_ERR;
    }
    idx = off / (uint32_t)vec->size;
#else
    off = (uint64_t)(q - p);
    if(off % (uint64_t)vec->size != 0){
        return MSD_ERR;
    }
    idx = off / (uint64_t)vec->size;
#endif

    return idx;
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
                
    if (!temp) {
        return MSD_ERR;
    }
    
    vec->data = temp;
    vec->slots *= 2;
    return MSD_OK;
}

/**
 * 功能: vector set at
 * 参数: @vec, @index
 *       @data, 待插入元素地址
 * 注意:
 *       本函数对count的处理是有可能存在问题的，set_at有可能是在清空!
 *       所以Vector中的count只能是一个参考值，他是最后一个有效元素后面的索引
 * 返回: 成功 0 失败 -x
 **/
int msd_vector_set_at(msd_vector_t *vec, unsigned int index, void *elem) 
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
    
    memcpy((char *)vec->data + (index * vec->size), elem, vec->size);

    if(index >= vec->count)
        vec->count= index+1; 
    return MSD_OK;
} 
 
/**
 * 功能: 在vector尾部添加一个元素
 * 参数: @vec
 *       @elem，待插入元素地址
 * 描述:
 *      1. vector索引从0开始
 * 返回: 成功 0 失败 -x
 **/  
int msd_vector_push(msd_vector_t *vec, void *elem) 
{
    if (vec->count == (vec->slots)) 
    {
        if (msd_vector_resize(vec) != MSD_OK){
            return MSD_ERR;
        }
    }
    
    return msd_vector_set_at(vec, vec->count, elem);
}

 /**
  * 功能: pop a element
  * 参数: @vec
  * 描述:
  *      1. 索引从0开始，注意边界条件
  * 返回: 成功，element地址，失败，NULL
  **/
void *msd_vector_pop(msd_vector_t *vec)
{
    void *elem;

    if(vec->count <= 0){
        return NULL;
    }
 
    vec->count--;
    elem = (uint8_t *)vec->data + vec->size * vec->count;
 
    return elem;
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
 * 功能: 获得栈顶元素，但是不pop
 * 参数: @vec
 * 返回: 成功，element地址，失败，NULL
 **/
void *msd_vector_top(msd_vector_t *vec)
{
    return msd_vector_get_at(vec, vec->count - 1);
}

/**
 * 功能: 按照给定比较函数，对数组的元素进行升序排序
 * 参数: @vec, @cmp
 * 描述:
 *       void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));
 *
 * DESCRIPTION:
 *       The qsort() function sorts an array with nmemb elements of size size.  The base argument points to the start of the array.
 *       The contents of the array are sorted in ascending order according to a comparison function pointed to by compar, which is called with two arguments that point to the objects being compared.
 *       The  comparison function must return an integer less than, equal to, or greater than zero if the first argument is considered to be respectively less than, equal to, or greater than the second.  If two mem-
 *       bers compare as equal, their order in the sorted array is undefined.
 * 返回: 成功，0，失败，-1
 **/
int msd_vector_sort(msd_vector_t *vec, msd_vector_cmp_t cmp)
{
    if(vec->count == 0){
        return MSD_ERR;
    }
    qsort(vec->data, vec->count, vec->size, cmp);
    return MSD_OK;
}

/**
 * 功能: 对数组的每个元素，执行特定操作
 * 参数: @vec, @func, 
 *       @data，函数func的外带参数
 * 描述:
 *      1. 如果某个元素执行失败，则直接返回错误
 * 返回: 成功，0，失败，NULL
 **/
int msd_vector_each(msd_vector_t *vec, msd_vector_each_t func, void *data)
{
    uint32_t i;
    int  rc;
    void *elem;

    if(vec->count == 0 || func==NULL){
        return MSD_ERR;
    }
    
    for (i = 0; i < vec->count; i++) 
    {
        elem = msd_vector_get_at(vec, i);

        rc = func(elem, data);
        if (rc != MSD_OK){
            return rc;
        }
    }

    return MSD_OK;
}

#ifdef __MSD_VECTOR_TEST_MAIN__
#define COUNT       20
typedef struct _test{
    int a;
    int b;
}test_t;

int my_cmp(const void* val1, const void* val2)
{
     test_t *p1, *p2;
     p1 = (test_t*)val1;
     p2 = (test_t*)val2;

     return (p1->b - p2->b); 
}

int my_print(void* elem, void* data)
{
    test_t        *p1;
    int            idx;
    msd_vector_t  *vec;
    
    p1  = (test_t*)elem; 
    vec = (msd_vector_t*)data;
    idx = msd_vector_idx(vec, elem);
    
    printf("idx=%d, value.a=%d, value.b=%d\n", idx, p1->a, p1->b);
    return MSD_OK;
}

int main(int argc, char **argv) 
{
    int i = 0;
    test_t t ,*value;
    
    msd_vector_t *vec = msd_vector_new(32, sizeof(test_t));
    for (i = 0; i < COUNT; ++i) 
    {
        t.a = i;
        t.b = 100-i;
        msd_vector_push(vec, &t);
    }

    //插入重复值
    t.a = 3;t.b = 97;
    msd_vector_push(vec, &t);
    msd_vector_push(vec, &t);
    /* 
     // DUMP
     for (i = 0; i < vec->count; ++i) 
     {
        value = (test_t *)msd_vector_get_at(vec, i);
        printf("idx=%d, value.a=%d, value.b=%d\n",i, value->a, value->b);
     }
     */

    //指定索set
    t.a = 3;t.b = 97;
    msd_vector_set_at(vec, 5, &t);   
    
    msd_vector_each(vec, my_print, vec);
    printf("slots:%d\n", vec->slots);
    printf("count:%d\n\n\n", vec->count);

    //排序
    msd_vector_sort(vec, my_cmp);
    msd_vector_each(vec, my_print, vec);
    printf("slots:%d\n", vec->slots);
    printf("count:%d\n\n\n", vec->count);

    value = msd_vector_pop(vec);
    printf("value->a:%d\n", value->a);
    printf("value->b:%d\n\n\n", value->b);
    
    msd_vector_free(vec);
    return 0;
}
#endif /* _MSD_VECTOR_TEST_MAIN__ */

