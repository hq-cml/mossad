DEBUG   = -g -DDEBUG
CC      = gcc
CFLAGS  = $(DEBUG) -Wall
INC     = -I../../inc/
LIB     = -lpthread
BENCHOO = msd_pressure_test.o msd_dlist.o msd_ae.o msd_string.o msd_anet.o msd_log.o msd_lock.o
BENCH   = EchoBenchmark

all: $(BENCH)

$(BENCH): $(BENCHOO)
	$(CC) $(CFLAGS) $(BENCHOO) $(LIB) -o $@

%.o:%.c
	$(CC) $(CFLAGS) $< -c -o $@ $(INC)

clean:
	rm -f msd_pressure_test.o
	rm -f $(BENCH)
