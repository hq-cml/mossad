#DEBUG  = -g -D__MSD_STRING_TEST_MAIN__
DEBUG  = -g 
CC     = gcc
CFLAGS = $(DEBUG) -Wall
LIB    = -ldl -rdynamic -lpthread
INC    = -I../inc
INC_DIR= ../inc/

TARGET = mossad 
#TARGET = msd_string 
#OBJECT = msd_string.o
OBJECT = msd_conf.o msd_hash.o msd_string.o msd_vector.o msd_dlist.o\
		 msd_lock.o msd_log.o msd_ae.o msd_so.o msd_daemon.o msd_anet.o\
		 msd_thread.o msd_master.o\
		 mossad.o

all: $(TARGET) 

#$(OBJECT): %:%.c $(INC_DIR)%.h
#	$(CC) $(CFLAGS) $(INC) $< -o $@

$(OBJECT): %.o:%.c $(INC_DIR)%.h
	$(CC) $(CFLAGS) $(INC) $< -c -o $@

$(TARGET):$(OBJECT)
	$(CC) $(CFLAGS) $(INC) $(LIB) $(OBJECT) -o $@
	rm -f *.o
#deps
msd_conf.o: msd_string.c msd_hash.c

.PHONY :clean install
install:
	install $(TARGET) ../bin/
	install mossad.conf.default ../bin/mossad.conf
clean:
	rm -f *.o
	rm $(TARGET) 
