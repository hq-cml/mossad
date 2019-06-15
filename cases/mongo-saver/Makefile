CC = gcc

# 锁模式：
# 线程mutex(默认) | SYSVSEM | 文件锁 
# [__MSD_PTHREAD_LOCK_MODE__ / __MSD_SYSVSEM_LOCK_MODE__ / __MSD_FCNTL_LOCK_MODE__]
LOCK_MODE = -D__MSD_PTHREAD_LOCK_MODE__

CFLAGS  = $(LOCK_MODE) -fPIC -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -D_REENTRANT
LIB     = -ldl -lrt -lm -lcurl -rdynamic -lcJSON -L.
INC     = -I../../inc -I../../lib/cJSON/
MONGO   = $(shell /usr/bin/pkg-config --cflags --libs libmongoc-1.0)

OO  = mongo_saver.o
TARGETS = libmongo_saver.so

all: $(TARGETS)

$(TARGETS): $(OO)
	$(CC) $(MONGO) $(CFLAGS) $(OO) -shared -o $@ $(LIBDIR) $(LIB) 

install:
	install $(TARGETS) ../../bin/mongo_saver/
	cp mongo_saver.conf ../../bin/mongo_saver/

.c.o:
	$(CC) $(MONGO) $(CFLAGS) $< -c -o $@ $(INC) 

clean:
	rm -f *.o
	rm -f $(TARGETS)
