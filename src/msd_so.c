/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *     Filename:  Msd_so.h 
 * 
 *  Description:  A dynamic library implemention. 
 * 
 *      Created:  May 14, 2012 
 *      Version:  0.0.1 
 * 
 *       Author:  HQ 
 *      Company:  Qihoo 
 *
 **/
#include "msd_core.h"
 
/**
 * 功能: 根据参数sym数组里面的顺序，逐个个加载函数，到全局dll中
 * 参数: @phandle，动态库句柄的指针
 *       @sym，smd_symbol_t数组
 *       @filename，so文件位置
 * 注意:
 *      1. 遍历sym数组，分别初始化全局dll中的各个函数
 * 返回: 成功，0 失败，-x
 **/
int msd_load_so(void **phandle, msd_so_symbol_t *sym, const char *filename) 
{
    char    *error;
    int     i = 0;
    
    *phandle = dlopen(filename, RTLD_NOW);
    if ((error = dlerror()) != NULL) 
    {
        fprintf(stderr, "dlopen so file failed:%s\n", error);
        return MSD_ERR;
    }

    while (sym[i].sym_name) 
    {
        if (sym[i].no_error) 
        {
            //不关心dlsym是否失败，若失败则只打印错误信息 
            *(void **)(sym[i].sym_ptr) = dlsym(*phandle, sym[i].sym_name);
            dlerror();
        } 
        else 
        {
            //若dlsym失败，会关闭句柄，释放资源
            *(void **)(sym[i].sym_ptr) = dlsym(*phandle, sym[i].sym_name);
            if ((error = dlerror()) != NULL) 
            { 
                dlclose(*phandle); 
                *phandle = NULL;
                return MSD_ERR;
            }          
        }
        ++i;
    }

    return MSD_OK;
}

/*
 * 功能: close 句柄
 */
void msd_unload_so(void **phandle) 
{
    if (*phandle != NULL) 
    {
        dlclose(*phandle);
        *phandle = NULL;
    }
}

#ifdef __MSD_SO_TEST_MAIN__

//动态库类型，成员是动态库包含的函数原型
typedef struct dll_func_struct 
{
    int (*handle_init)(const void *data, int proc_type);
    int (*handle_fini)(const void *data, int proc_type);
    int (*handle_task)(const void *data);
} dll_func_t;

dll_func_t dll;

//与动态库一一对应的标记数组，包含了每个动态库函数的具体信息
msd_so_symbol_t syms[] = 
{
    {"handle_init", (void **)&dll.handle_init, 1},
    {"handle_fini", (void **)&dll.handle_fini, 1},
    {"handle_task", (void **)&dll.handle_task, 0},
    {NULL, NULL, 0}
};

int main(int argc, char *argv[]) 
{
    void * handle;

    if (argc < 2) 
    {
        fprintf(stderr, "Invalid arguments\n");
        exit(1);
    }

    if (msd_load_so(&handle, syms, argv[1]) < 0) 
    {
        fprintf(stderr, "load so file failed\n");
        exit(1);
    }

    if (dll.handle_init) 
    {
        dll.handle_init("handle_init", 0);
    }

    dll.handle_task("handle_task");

    if (dll.handle_fini) 
    {
        dll.handle_fini("handle_fnit", 1);
    }

    msd_unload_so(&handle);
    exit(0);
}
#endif /* __MSD_SO_TEST_MAIN__ */
