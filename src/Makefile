DEBUG  = -g -D__MSD_LOCK_TEST_MAIN__
CC     = gcc
CFLAGS = $(DEBUG) -Wall
LIB    = -ldl -rdynamic -lpthread
INC    = -I../inc
INC_DIR= ../inc/

TARGET = msd_lock
OBJECT = msd_conf.o msd_hash.o msd_string.o msd_vector.o msd_dlist.o\
		 msd_lock.o

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

.PHONY :clean
clean:
	rm -f *.o
	rm $(TARGET) 
