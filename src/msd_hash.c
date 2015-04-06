/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_hash.c 
 * 
 * Description :  Msd_hash, a simple hash library. 
 *                Maybe it's not complex than redis's dict, but is enough.
 * 
 *     Version :  1.0.0
 *      
 *      Author :  HQ 
 *
 **/

//#include "msd_hash.h"
#include "msd_core.h"

/**
 * 功能: 默认set函数
 **/
void *msd_hash_def_set(const void *key){    return strdup((char *)key); }

/**
 * 功能: 默认free函数
 **/
void msd_hash_def_free(void *key){ if(!key) free(key); }

/**
 * 功能: 默认cmp函数
 **/
int msd_hash_def_cmp(const void *key1, const void *key2) { return strcmp((char *)key1, (char *)key2) == 0; }

/**
 * 功能: get the minimun 2'n which is biger than size
 * 参数: @size:初始给定大小 
 * 返回: the minimun 2'n which is biger than size
 **/
static unsigned long msd_next_power(unsigned long size)
{
    unsigned long i = MSD_HASH_INIT_SLOTS;

    if(size >= LONG_MAX)
    {
        return LONG_MAX; /* 4*1024*1024*1024 */
    }

    while(1)
    {
        if(i >= size)
        {
            return i;
        }
        i *= 2;
    }
}

/**
 * 功能: hash散列函数
 * 参数: @key  
 * 返回: 散列结果
 **/
static unsigned int msd_hash_func(const void *key)
{
    unsigned int h = 5381;
    char *ptr = (char *)key;

    while(*ptr != '\0')
    {
        h = (h+(h<<5)) + (*ptr++);
    }
    return h;
}

/**
 * 功能: nitialize an allocated hash table
 * 参数: @ht, 已经分配好空间了的hash结构
 *       @slots, 槽位数 
 * 返回: 成功， 0 失败，-x
 **/
static int msd_hash_init(msd_hash_t *ht, unsigned int slots)
{
    assert(ht);

    ht->slots = msd_next_power(slots);
    ht->count = 0;

    ht->data = (msd_hash_entry_t **)calloc(ht->slots, sizeof(msd_hash_entry_t *));
    if(!ht->data)
    {
        return MSD_FAILED;
    }
    return MSD_OK;
}

/**
 * 功能: create a new hash table and init which immediately
 * 参数: @slots,初始槽位数
 * 返回:成功，msd_hash_t地址，失败，NULL
 **/
msd_hash_t *msd_hash_create(unsigned int slots)
{
    msd_hash_t *ht = calloc(1, sizeof(*ht));
    if(!ht)
    {
        return NULL;
    }

    if((msd_hash_init(ht, slots) != MSD_OK))
    {
        free(ht);
        ht = NULL;
        return NULL;
    }

    return ht;
}

/**
 * 功能: double site of the table when not enough slots
 * 参数: @ht, 待扩容的hash结构地址
 * 描述:
 *      1. static
 * 返回: 成功， 0 失败，-x
 **/
static int msd_hash_resize(msd_hash_t *ht)
{
    msd_hash_entry_t **tmp, *he, *next;
    unsigned int i, old_slots, h;
    int ret;

    tmp = ht->data;
    ht->data = NULL;
    old_slots = ht->slots;

    /*扩容两倍*/
    if((ret = msd_hash_init(ht, old_slots*2)) !=0 )
    {
        return MSD_FAILED;
    }

    /*老hash中的元素挨个插入*/
    for(i=0; i<old_slots; i++)
    {
        if((he = tmp[i]))
        {
            while(he)
            {
                next = he->next;
                h = msd_hash_func(he->key)%ht->slots; /* new hash value */
                /* insert at the head */
                he->next = ht->data[h];
                ht->data[h] = he;
                ht->count ++;
                he = next;
            }
        }
    }

    free(tmp);
    return MSD_OK;
}

/**
 * 功能: insert a key/value pair into a hash table 
 * 参数:@ 
 * 描述:
 *      1. 如果key存在，则替换，否则插入
 * 返回:
 **/
int msd_hash_insert(msd_hash_t *ht, const void *key, const void *val)
{
    msd_hash_entry_t *he;
    unsigned int hash, i;

    assert(ht && key && val);
    
    /* if the slots userate > RESIZE_RATIO, then resize */
    if((ht->count / ht->slots) >= MSD_HASH_RESIZE_RATIO)
    {
        //TODO LOG
        //printf("Resize, now slots:%d\n", ht->slots);
        if(msd_hash_resize(ht) != MSD_OK)
        {
            return MSD_ERR;
        }
    }

    hash = msd_hash_func(key);
    i    = hash % ht->slots;

    if((he = ht->data[i]))
    {
        while(he)
        {
            if(MSD_HASH_CMP_KEYS(ht, key, he->key))
            {
                /* exist same key, then replace the value */
                //TODO LOG
                //printf("same\n");
                MSD_HASH_FREE_VAL(ht, he);
                MSD_HASH_SET_VAL(ht, he, (void *)val);
                return MSD_OK;
            }
            he = he->next;
        }
    }

    he = calloc(1, sizeof(*he));/* sizeof(msd_hash_entry_t) ? */
    if(!he)
    {
        return MSD_FAILED;
    }

    MSD_HASH_SET_KEY(ht, he, (void *)key);
    MSD_HASH_SET_VAL(ht, he, (void *)val);

    /* insert before head */
    he->next = ht->data[i];
    ht->data[i] = he;

    ++ht->count;
    return MSD_OK;
}

/**
 * 功能: remove an entry from a hash table
 * 参数: @ht,hash结构体地址
 *       @key，欲remove的key
 * 描述:
 *      1. 防止内存泄露，需要分别调用FREE_KEY\FREE_VAL\free(he)
 * 返回:成功，0，失败, -x
 **/
int msd_hash_remove_entry(msd_hash_t *ht, const void *key)
{
    unsigned int index;
    msd_hash_entry_t *he, *prev;

    index = msd_hash_func(key) % ht->slots;

    if((he=ht->data[index]))
    {
        prev = NULL;
        while(he)
        {
            if(MSD_HASH_CMP_KEYS(ht, key, he->key))
            {
                /* remove */
                if(prev)
                {
                    prev->next = he->next;
                }
                else
                {
                    ht->data[index] = he->next;/*第一个元素*/
                }

                MSD_HASH_FREE_KEY(ht, he);
                MSD_HASH_FREE_VAL(ht, he);
                free(he);
                --ht->count;
                return MSD_OK;
            }

            prev = he;
            he = he->next;
        }
    }

    return MSD_NONEED;
}

/**
 * 功能: execute the provided function fro each entry
 * 参数: @ht, hash结构体地址
 *       @foreach, 函数指针，将作用于每一个entry
 *                 第一个参数是entry指针，第二个参数是外带数据
 *       @userptr, 外带数据，用户传递给foreach
 * 描述: 
 *      1. 遍历每一个entry，分别执行给定函数
 * 返回: 成功，0,失败，-x
 **/
int msd_hash_foreach(msd_hash_t *ht,
        int (*foreach)(const msd_hash_entry_t *, void *userptr),
        void *userptr)
{
    unsigned int i;
    msd_hash_entry_t *he;

    assert(foreach);
    
    /* 遍历每一个entry */
    for(i=0; i<ht->slots; i++)
    {
        if((he=ht->data[i]))
        {
            while(he)
            {
                if((foreach(he, userptr) != MSD_OK ))
                {
                    return MSD_ERR;
                }
                he = he->next;
            }
        }
    }
    
    return MSD_OK;
}

/**
 * 功能: get a value by key
 * 参数: @ht,hash结构体地址
 *       @key，欲查询key
 * 描述:
 *      1. 根据hash算法，找到对应槽位，然后此槽位上遍历每个entry
 * 返回: 成功，返回val的地址，失败NULL
 **/
void *msd_hash_get_val(msd_hash_t *ht, const void *key)
{
    unsigned int hash, i;
    msd_hash_entry_t *he;

    hash = msd_hash_func(key);
    i = hash % ht->slots;

    if((he = ht->data[i]))
    {
        while(he)
        {
            if(MSD_HASH_CMP_KEYS(ht, key, he->key))
            {
                return he->val;
            }
            he = he->next;
        }
    }
    return NULL;
}

/**
 * 功能: the foreach function to be used by qbh_hash_destroy
 * 参数: @ he,待删除的entry地址
 *       @ ht，hash结构地址
 * 描述:
 *      1. 该函数的存在，主要是因为msd_hash_foreach的第二参数
 *         是一个需要第一个参数是个he的函数
 * 返回: 成功，0,失败，-x
 **/
static int msd_hash_delete_foreach(const msd_hash_entry_t *he, void *ht)
{
    if(msd_hash_remove_entry(ht, he->key) != MSD_OK)
    {
        return MSD_ERR;
    } 
    
    return MSD_OK;
}

/**
 * 功能: clear a hash table
 * 参数: @ hash结构地址
 * 描述:
 *      1. 销毁hash的数据，但是hash结构本身不销毁
 *      2. 并且ht->data堆数组不销毁，但会清0
 **/
void msd_hash_clear(msd_hash_t *ht)
{
    msd_hash_foreach(ht, msd_hash_delete_foreach, ht); /*将每个元素清除*/
    memset(ht->data, 0, ht->slots * sizeof(msd_hash_entry_t *)); 
    ht->count = 0;
}

/**
 * 功能: destroy a hash table
 * 参数: @ hash结构地址
 * 描述:
 *      1. 销毁hash的数据，但是hash结构本身不销毁
 **/
void msd_hash_destroy(msd_hash_t *ht)
{
    msd_hash_foreach(ht, msd_hash_delete_foreach, ht); /*将每个元素清除*/
    free(ht->data); /*data数组清除*/
    ht->data = NULL;
    ht->count = 0;
    ht->slots = 0;
    
    ht->keycmp = NULL;
    ht->set_key = NULL;
    ht->set_val = NULL;
    ht->free_key = NULL;
    ht->free_val = NULL;
}
/**
 * 功能: free a hash table
 * 参数:@ hash结构地址
 * 描述:
 *      1. 销毁全部，包括hash结构本身
 **/
void msd_hash_free(msd_hash_t *ht)
{
    msd_hash_destroy(ht);/*ht结构清除，杜绝泄露*/
    free(ht);
}

/**
 * 功能: the froeach handler to duplicate the hash table
 **/
static int msd_hash_dup_foreach(const msd_hash_entry_t *he, void *copy)
{
    if(msd_hash_insert((msd_hash_t *)copy, he->key, he->val) != MSD_OK)
    {
        return MSD_ERR;
    }

    return MSD_OK;
}

/**
 * 功能: duplicate a hash table
 * 参数: @ 待拷贝哈希结构地址
 * 返回: 成功，新hash结构地址，失败NULL
 **/
msd_hash_t *msd_hash_duplicate(msd_hash_t *ht)
{
    msd_hash_t *copy;
    copy = msd_hash_create(ht->slots);
    if(!copy)
    {
        return NULL;
    }

    MSD_HASH_SET_SET_KEY(copy, ht->set_key);
    MSD_HASH_SET_SET_VAL(copy, ht->set_val);
    MSD_HASH_SET_FREE_KEY(copy, ht->free_key);
    MSD_HASH_SET_FREE_VAL(copy, ht->free_val);
    MSD_HASH_SET_KEYCMP(copy, ht->keycmp);

    msd_hash_foreach(ht, msd_hash_dup_foreach, copy);
    return copy;
}

/**
 * 功能: init a new hash iterator struct
 **/
static int msd_hash_iter_init(msd_hash_iter_t *iter, msd_hash_t *ht)
{
    iter->pos   = 0;
    iter->depth = 0;
    iter->ht    = ht;
    iter->he    = NULL;
    iter->key   = NULL;
    iter->val   = NULL;
    return msd_hash_iter_move_next(iter);
}

/**
 * 功能: create a new hash iteratior, and then init it
 * 参数: @ ht, hash结构体地址
 * 返回: 成功，iter结构体地址，失败，NULL
 **/
msd_hash_iter_t *msd_hash_iter_new(msd_hash_t *ht)
{
    msd_hash_iter_t *iter = (msd_hash_iter_t *)calloc(1, sizeof(*iter));
    if(!iter)
    {
        return NULL;
    }

    if(msd_hash_iter_init(iter, ht) != 0)
    {
        free(iter);
        return NULL;
    }
    return iter;
}

/**
 * 功能: move to the next position in the table
 * 参数: @ iter
 * 描述:
 *      1. 如果iter处于某个槽位对应的链表的最后位置，则下一个位置
 *         必然是位于下一个飞空槽位的起始位置
 * 返回: 成功，0，失败-x
 **/
int msd_hash_iter_move_next(msd_hash_iter_t *iter)
{
    if(iter->he)
    {
        if(iter->he->next)
        {
            iter->he = iter->he->next;
            iter->key = iter->he->key;
            iter->val = iter->he->val;
            iter->depth++;
            return MSD_OK;
        }
        else /* iter处于某个槽位对应的链表的最后位置 */
        {
            iter->pos++;
        }
    }

    /* reset the depth */
    iter->depth = 1;
    for(; iter->pos < iter->ht->slots; iter->pos++)
    {
        if((iter->he = iter->ht->data[iter->pos]))
        {
            iter->key = iter->he->key;
            iter->val = iter->he->val;
            return MSD_OK;
        }
    }

    return MSD_END;
}

/**
 * 功能: move to the prev position in the table
 * 参数: @ iter 
 * 描述:
 *      1. 这个函数还有点问题有潜在bug!!!
 * 返回:成功，0，失败-x
 **/
int msd_hash_iter_move_prev(msd_hash_iter_t *iter)
{
    int i;
    if(iter->depth>0 && iter->pos<iter->ht->slots)
    {
        for(iter->he = iter->ht->data[iter->pos], i=0; i < iter->depth-2; i++)
        {
            iter->he = iter->he->next;
        }
        iter->depth--;
        if(iter->depth>0)
        {
            iter->key = iter->he->key;
            iter->val = iter->he->val;
            return MSD_OK;
        }    
    }
    
    iter->depth = 0;
    --iter->pos;
    for(; iter->pos; --iter->pos)
    {
        if((iter->he = iter->ht->data[iter->pos]))
        {
            iter->depth++;
            while(iter->he->next)
            {
                iter->he = iter->he->next;
                iter->depth++;
            }
            iter->key = iter->he->key;
            iter->val = iter->he->val;
            return MSD_OK;
        }
    }
    return MSD_FAILED;
}

/**
 * 功能:destroy a hash iterator
 **/
static void msd_hash_iter_destroy(msd_hash_iter_t *iter)
{
    iter->pos = 0;
    iter->depth = 0;
    iter->ht = NULL;
    iter->he = NULL;
    iter->key = NULL;
    iter->val = NULL;
}

/**
 * 功能: reset the iter
 **/
int msd_hash_iter_reset(msd_hash_iter_t *iter)
{
    msd_hash_t *tmp;
    tmp = iter->ht;
    msd_hash_iter_destroy(iter);
    return msd_hash_iter_init(iter, tmp);
}
 
/**
 * 功能: free a hash iterator
 **/
void msd_hash_iter_free(msd_hash_iter_t *iter)
{
    msd_hash_iter_destroy(iter);
    free(iter);
}

#ifdef __MSD_HASH_TEST_MAIN__

static int _hash_print_entry(const msd_hash_entry_t *he, void *userptr)
{
    printf("%s => %s\n", (char *)he->key, (char *)he->val);
    return MSD_OK;
}

static void hash_dump(msd_hash_t *ht)
{
    printf("ht->slots:%d\n", ht->slots);
    printf("ht->count:%d\n", ht->count);
    msd_hash_foreach(ht, _hash_print_entry, NULL);
}

static int randstring(char *target, unsigned int min, unsigned int max)
{
    int p = 0;
    int len = min + rand() % (max - min + 1);
    int minval, maxval;
    minval = 'A';
    maxval = 'z';
    while (p < len)
    {
        target[p++] = minval + rand()%(maxval - minval + 1);
    }
    return len;
}

static void *_demo_dup(const void *key)
{
    return strdup((char *)key);
}

static int _demo_cmp(const void *key1, const void *key2)
{
    return strcmp((char*)key1, (char*)key2) == 0;
}

static void _demo_destructor(void *key)
{
    //printf("Free:%p\n", key);
    free(key);
}

int main()
{
    char buf1[32];
    char buf2[32];
    int i,j;
    int len;
    msd_hash_t *ht, *htcopy;
    msd_hash_iter_t *iter;
    /*
    printf("LONG_MAX: %u\n",LONG_MAX);
    printf("msd_next_power(50): %u\n",msd_next_power(50));

    char *tmp = "fuck bitch";
    printf("msd_hash_func('fuck bitch'): %u\n",msd_hash_func("fuck bitch"));
    printf("msd_hash_func(tmp): %u\n",msd_hash_func(tmp)); 
    */

    /*
    //test mem leak
    //一个奇怪现象，free应该是生效了，但是确不会改变top显示的消耗的内存量
    //当多批次申请的时候，如果中间不free可以看到内存一直在增长
    //如果free的话，则内存只会涨到每批次的峰值
    srand(time(NULL));
    ht = msd_hash_create(2);
    MSD_HASH_SET_SET_KEY(ht,  _demo_dup);
    MSD_HASH_SET_SET_VAL(ht,  _demo_dup);
    MSD_HASH_SET_FREE_KEY(ht, _demo_destructor);
    MSD_HASH_SET_FREE_VAL(ht, _demo_destructor);
    MSD_HASH_SET_KEYCMP(ht,   _demo_cmp);
    j = 1000000;
    for(i=0; i<j; i++)
    {
        len = randstring(buf1, 1, sizeof(buf1)-1);
        buf1[len] = '\0';
        len = randstring(buf2, 1, sizeof(buf2)-1);
        buf2[len] = '\0';
        msd_hash_insert(ht, buf1, buf2);
    }
    printf("\nbefore hash_destroy:\n");
    sleep(10);

    msd_hash_destroy(ht);
    printf("middle\n");
    ht = msd_hash_create(2);
    MSD_HASH_SET_SET_KEY(ht,  _demo_dup);
    MSD_HASH_SET_SET_VAL(ht,  _demo_dup);
    MSD_HASH_SET_FREE_KEY(ht, _demo_destructor);
    MSD_HASH_SET_FREE_VAL(ht, _demo_destructor);
    MSD_HASH_SET_KEYCMP(ht,   _demo_cmp);    

    for(i=0; i<j; i++)
    {
        len = randstring(buf1, 1, sizeof(buf1)-1);
        buf1[len] = '\0';
        len = randstring(buf2, 1, sizeof(buf2)-1);
        buf2[len] = '\0';
        msd_hash_insert(ht, buf1, buf2);
    }
    printf("\nhash_destroy:\n");
    //hash_dump(ht);
    sleep(30);
    */

    
    //j = 10000;
    j = 10;
    srand(time(NULL));
    ht = msd_hash_create(2);
    MSD_HASH_SET_SET_KEY(ht,  _demo_dup);
    MSD_HASH_SET_SET_VAL(ht,  _demo_dup);
    MSD_HASH_SET_FREE_KEY(ht, _demo_destructor);
    MSD_HASH_SET_FREE_VAL(ht, _demo_destructor);
    MSD_HASH_SET_KEYCMP(ht,   _demo_cmp);

    for(i=0; i<j; i++)
    {
        len = randstring(buf1, 1, sizeof(buf1)-1);
        buf1[len] = '\0';
        len = randstring(buf2, 1, sizeof(buf2)-1);
        buf2[len] = '\0';
        msd_hash_insert(ht, buf1, buf2);
    }
    msd_hash_insert(ht, "test", "aaa");
    printf("\nhash_dump:\n");
    hash_dump(ht);
    //printf("Get a val:%s=>%s\n", buf1, msd_hash_get_val(ht, buf1));
    //msd_hash_remove_entry(ht, buf1);
    //hash_dump(ht);
    printf("\nhash_duplicate:\n");
    htcopy = msd_hash_duplicate(ht);
    //hash_dump(htcopy);
    //msd_hash_destroy(ht);
    printf("The test key:%s\n", msd_hash_get_val(ht, "test"));    
    printf("The test key:%s\n", msd_hash_get_val(ht, "test1"));    

    iter = msd_hash_iter_new(htcopy);
    assert(iter);    
    
    printf("\nbefore iter_next\n");
    do{
        printf("%s => %s\n", iter->key, iter->val);
    }while(msd_hash_iter_move_next(iter)==MSD_OK);
    
    printf("\niter->pos:%d\n", iter->pos);
    printf("iter->depts:%d\n\n", iter->depth);
    
    
    printf("\nbefore iter_prev\n");
    while(msd_hash_iter_move_prev(iter)==MSD_OK){
        printf("%s => %s\n", iter->key, iter->val);
    }    
    
    msd_hash_destroy(htcopy);
    msd_hash_iter_free(iter);

    return 0;
}
#endif /* __MSD_HASH_TEST_MAIN__ */

