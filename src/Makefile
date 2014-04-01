DEBUG  = -g -D__MSD_CONF_TEST_MAIN__
CC     = gcc
CFLAGS = $(DEBUG) -Wall
LIB    = -ldl -rdynamic
INC    = -I../inc
INC_DIR= ../inc/

TARGET = msd_conf
OBJECT = msd_conf.o msd_hash.o msd_string.o

all: $(TARGET)

#$(OBJECT): %:%.c $(INC_DIR)%.h
#   $(CC) $(CFLAGS) $(INC) $< -o $@

$(OBJECT): %.o:%.c $(INC_DIR)%.h
    $(CC) $(CFLAGS) $(INC) $< -c -o $@

$(TARGET): $(OBJECT)
    $(CC) $(CFLAGS) $(INC) $< -o $@

#deps
msd_conf.o: msd_string.c msd_hash.c

.PHONY :clean
clean:
    rm -f *.o
    rm $(TARGET) 
