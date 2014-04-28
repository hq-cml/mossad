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
#ifndef __MSD_DLL_H_INCLUDED__ 
#define __MSD_DLL_H_INCLUDED__

/*
#define MSD_OK  0
#define MSD_ERR -1
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
*/
 
typedef struct msd_symbol_struct 
{
    char    *sym_name; /* 动态库中某个函数的名称 */
    void    **sym_ptr; /* 动态库中某个函数的地址 */
    int     no_error;  /* 如果为1，则不关心dlsym是否失败；若为0，则dlsym失败后会释放句柄 */
} msd_symbol_t;

int msd_load_so(void **phandle, msd_symbol_t *syms, const char *filename);
void msd_unload_so(void **phandle);

#endif /* __MSD_DLL_H_INCLUDED__ */
