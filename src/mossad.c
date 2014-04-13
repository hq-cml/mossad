/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  / \  |  _ \ 
 * | |\/| | | | \___ \___ \ / _ \ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Mossad.c
 * 
 * Description :  Mossad main logic。
 * 
 *     Created :  Apr 11, 2012 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *     Company :  Qh 
 *
 **/

#include "msd_core.h"

/* 长参数列表 */
static struct option const long_options[] = {
    {"config",  required_argument, NULL, 'c'},
    {"help",    no_argument,       NULL, 'h'},
    {"version", no_argument,       NULL, 'v'},
    {"info",    no_argument,       NULL, 'i'},
    {NULL, 0, NULL, 0}
};

msd_instance_t *g_ins;

/**
 * 功能: 初始化instance
 * 返回：成功：instance 指针
 **/
static msd_instance_t * msd_create_instance()
{
    msd_instance_t *instance;
    instance = malloc(sizeof(msd_instance_t));
    if(!instance)
    {
        MSD_BOOT_FAILED("Init instance faliled!");
    }
    
    if(!(instance->conf_path = msd_str_new_empty()))
    {
        MSD_BOOT_FAILED("Init conf_path faliled!");
    }
    
    if(!(instance->conf = malloc(sizeof(msd_conf_t))))
    {
        MSD_BOOT_FAILED("Init malloc conf faliled!");
    }    

    instance->log       = NULL;
    
    return instance;
}

/**
 * 功能: 销毁instance
 **/
static void msd_destroy_instance()
{
    //TODO
}

/**
 * 功能: 打印"关于"信息
 **/
static void msd_print_info() 
{
    printf("[%s]: A async network server framework.\n"
           "Version: %s\n"
           "Copyright(c): hq, hq_cml@163.com\n"
           "Compiled at: %s %s\n", MSD_PROG_TITLE, MOSSAD_VERSION, 
           __DATE__, __TIME__);
}

/**
 * 功能: 打印"版本"信息
 **/
static void msd_print_version()
{
    printf("Version: %s\n", MOSSAD_VERSION);
}

/**
 * 功能：打印usage
 **/
static void msd_usage(int status) 
{
    if (status != EXIT_SUCCESS) 
    {
        fprintf(stderr, "Try '%s --help' for more information.\n", MSD_PROG_NAME);
    } 
    else 
    {
        printf("Usage:./%6.6s [--config=<conf_file> | -c] [start|stop]\n"
               "%6.6s         [--version | -v]\n"
               "%6.6s         [--help | -h]\n", MSD_PROG_NAME, " ", " ");
    }
}

/**
 * 功能：解析选项
 **/
static void msd_get_options(int argc, char **argv) 
{
    //char a[111];
    int c;
    while ((c = getopt_long(argc, argv, "c:hvi", 
                    long_options, NULL)) != -1) 
    {
        switch (c) 
        {
        case 'c':
            g_ins->conf_path = msd_str_cpy(g_ins->conf_path, optarg);
            break;
        case 'h':
            msd_usage(EXIT_SUCCESS);
            exit(EXIT_SUCCESS);
            break;
        case 'i':
            msd_print_info();
            exit(EXIT_SUCCESS);
            break;
        case 'v':
            msd_print_version();
            exit(EXIT_SUCCESS);
            break;            
        default:
            msd_usage(EXIT_FAILURE);
            exit(1);
        }
    }

    if (optind == argc) 
    {
        //daemon_action = MSD_DAEMON_START;
    } 
    else 
    {
        msd_usage(EXIT_FAILURE);
        exit(1);
    }
}

/*------------------------ Main -----------------------*/
int main(int argc, char **argv)
{
    g_ins = msd_create_instance();
    
    msd_get_options(argc, argv);
    
    /* 初始化conf */
    if(g_ins->conf_path->len == 0)
    {
        g_ins->conf_path = msd_str_cpy(g_ins->conf_path, "./mossad.conf");
        if(! g_ins->conf_path)
        {
            MSD_BOOT_FAILED("Init conf_path faliled!");
        }
    }    
    if(MSD_OK != msd_conf_init(g_ins->conf, g_ins->conf_path->buf))
    {
        MSD_BOOT_FAILED("Init conf faliled!");
    }
    MSD_BOOT_SUCCESS("INIT CONF SUCCESS");

    /* 日志初始化 */
    if (msd_log_init(msd_conf_get_str_value(g_ins->conf, "log_path", "./"),
            msd_conf_get_str_value(g_ins->conf, "log_name", MSD_PROG_NAME".log"),
            msd_conf_get_int_value(g_ins->conf, "log_level", MSD_LOG_LEVEL_ALL),
            msd_conf_get_int_value(g_ins->conf, "log_size", MSD_LOG_FILE_SIZE),
            msd_conf_get_int_value(g_ins->conf, "log_num", MSD_LOG_FILE_NUM),
            msd_conf_get_int_value(g_ins->conf, "log_multi", MSD_LOG_MULTIMODE_NO)) < 0) 
    {
        MSD_BOOT_FAILED("INIT LOG FAILED");
    }
    MSD_BOOT_SUCCESS("INIT LOG SUCCESS");
    MSD_INFO_LOG("Mossad begin to run");
    printf("Port:%d\n", msd_conf_get_int_value(g_ins->conf, "port", 9527));
    
    return 0;
}

















