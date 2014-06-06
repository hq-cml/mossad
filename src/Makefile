#DEBUG  = -g -D__MSD_SIGNAL_TEST_MAIN__
DEBUG  = -g 
CC     = gcc
CFLAGS = $(DEBUG) -Wall
LIB    = -ldl -rdynamic -lpthread
INC    = -I../inc
INC_DIR= ../inc/

TARGET    = Mossad 
TARG_PRES = EchoPress 

OBJECT = msd_conf.o msd_hash.o msd_string.o msd_vector.o msd_dlist.o\
		 msd_lock.o msd_log.o msd_ae.o msd_so.o msd_daemon.o msd_anet.o\
		 msd_thread.o msd_master.o msd_signal.o\
		 mossad.o
OBJ_PRES = msd_ae.o msd_anet.o msd_dlist.o msd_lock.o msd_log.o msd_string.o\
		   msd_pressure_test.o
OBJ_PRES_S = msd_pressure_test.o

all: $(TARGET) $(TARG_PRES) 

press: $(TARG_PRES) 

#不关心头文件，每次均执行make clean
#$(OBJECT): %.o:%.c $(INC_DIR)%.h
$(OBJECT): %.o:%.c 
	$(CC) $(CFLAGS) $(INC) $< -c -o $@

$(OBJ_PRES_S): %.o:%.c 
	$(CC) $(CFLAGS) $(INC) $< -c -o $@

$(TARGET):$(OBJECT)
	$(CC) $(CFLAGS) $(INC) $(LIB) $(OBJECT) -o $@

$(TARG_PRES):$(OBJ_PRES)
	$(CC) $(CFLAGS) $(INC) $(LIB) $(OBJ_PRES) -o $@

#deps
msd_conf.o: msd_string.c msd_hash.c
msd_signal.o: msd_log.c msd_lock.c

.PHONY :clean install
install:
	install $(TARGET) ../bin/echo/
	install $(TARG_PRES) ../tool/pressure_test/
	#install mossad.conf.default ../bin/mossad.conf

clean:
	rm -f *.o
	rm $(TARGET) 
