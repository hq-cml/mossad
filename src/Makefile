#DEBUG  = -g -D__MSD_SIGNAL_TEST_MAIN__
DEBUG  = -g 

# I/O复用模式：
# epoll(默认) | select 
# [__MSD_EPOLL_MODE__ / __MSD_SELECT_MODE__]
IO_MULTI_MODE = -D__MSD_EPOLL_MODE__

# 日志模式：
# 线程模式(默认) | 进程模式 
# [__MSD_LOG_MODE_THREAD__ / __MSD_LOG_MODE_PROCESS__]
LOG_MODE = -D__MSD_LOG_MODE_THREAD__

# 锁模式：
# 线程mutex(默认) | SYSVSEM | 文件锁 
# [__MSD_PTHREAD_LOCK_MODE__ / __MSD_SYSVSEM_LOCK_MODE__ / __MSD_FCNTL_LOCK_MODE__]
LOCK_MODE = -D__MSD_PTHREAD_LOCK_MODE__

CC     = gcc
CFLAGS = $(DEBUG) $(IO_MULTI_MODE) $(LOG_MODE) $(LOCK_MODE) -Wall
LIB    = -ldl -rdynamic -lpthread
INC    = -I../inc
INC_DIR= ../inc/

TARGET    = Mossad 

OBJECT = msd_conf.o msd_hash.o msd_string.o msd_vector.o msd_dlist.o\
		 msd_lock.o msd_log.o msd_ae.o msd_so.o msd_daemon.o msd_anet.o\
		 msd_thread.o msd_master.o msd_signal.o\
		 mossad.o

all: $(TARGET)         
         
#不关心头文件，每次均执行make clean
#$(OBJECT): %.o:%.c $(INC_DIR)%.h
$(OBJECT): %.o:%.c 
	$(CC) $(CFLAGS) $(INC) $< -c -o $@

$(TARGET):$(OBJECT)
	$(CC) $(CFLAGS) $(INC) $(LIB) $(OBJECT) -o $@

#deps
msd_conf.o: msd_string.c msd_hash.c
msd_signal.o: msd_log.c msd_lock.c

.PHONY :clean install
install:
	install $(TARGET) ../bin/redis_saver/
	#install mossad.conf.default ../bin/redis_saver/mossad.conf

clean:
	rm -f *.o
	rm $(TARGET) 
