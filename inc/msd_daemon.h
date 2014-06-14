/**
 *  __  __  ___  ____ ____    _    ____  
 * |  \/  |/ _ \/ ___/ ___|  /_\  |  _ \ 
 * | |\/| | | | \___ \___ \ //_\\ | | | |
 * | |  | | |_| |___) |__) / ___ \| |_| |
 * |_|  |_|\___/|____/____/_/   \_\____/ 
 *
 *    Filename :  Msd_daemon.h
 * 
 * Description :  Msd_daemon, daemon implementation.
 * 
 *     Version :  0.0.1 
 * 
 *      Author :  HQ 
 *
 **/

#ifndef __MSD_DAEMON_H_INCLUDED__
#define __MSD_DAEMON_H_INCLUDED__
 
/*
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>     

#define MSD_OK     0
#define MSD_ERR    -1
*/
#define MSD_MAX_FD 32768

char ** msd_set_program_name(int argc, char *argv[], const char *name);
void msd_daemon_argv_free(char **daemon_argv);

void msd_rlimit_reset();
void msd_redirect_std();
void msd_daemonize();

pid_t msd_pid_file_running(char *pid_file);
int msd_pid_file_create(char *pid_file);

//ÖÐÎÄ
#endif /* __MSD_DAEMON_H_INCLUDED__ */
