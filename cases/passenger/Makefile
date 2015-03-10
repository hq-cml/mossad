CC = gcc

# 锁模式：
# 线程mutex(默认) | SYSVSEM | 文件锁 
# [__MSD_PTHREAD_LOCK_MODE__ / __MSD_SYSVSEM_LOCK_MODE__ / __MSD_FCNTL_LOCK_MODE__]
LOCK_MODE = -D__MSD_PTHREAD_LOCK_MODE__

CFLAGS  = $(LOCK_MODE) -fPIC -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -D_REENTRANT
LIB     = -ldl -lrt -rdynamic
INC     = -I../../inc
OO  = passenger.o
TARGETS = libpassenger.so

all: $(TARGETS)

$(TARGETS): $(OO)
	$(CC) $(CFLAGS) $(OO) -shared -o $@ $(LIBDIR) $(LIB)

install:
	install $(TARGETS) ../../bin/passenger/
	cp passenger.conf ../../bin/passenger/

.c.o:
	$(CC) $(CFLAGS) $< -c -o $@ $(INC)

clean:
	rm -f *.o
	rm -f $(TARGETS)
