CC = gcc

# 锁模式：
# 线程mutex(默认) | SYSVSEM | 文件锁 
# [__MSD_PTHREAD_LOCK_MODE__ / __MSD_SYSVSEM_LOCK_MODE__ / __MSD_FCNTL_LOCK_MODE__]
LOCK_MODE = -D__MSD_PTHREAD_LOCK_MODE__

CFLAGS  = -fPIC -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -D_REENTRANT
LIB 	= -ldl -lrt -rdynamic
INC     = -I../../inc 
OO	= http.o
TARGETS = libhttp.so

all: $(TARGETS)

$(TARGETS): $(OO)
	$(CC) $(CFLAGS) $(OO) -shared -o $@ $(LIBDIR) $(LIB)

install:
	install $(TARGETS) ../../bin/httpd/
	cp http.conf ../../bin/httpd/
	cp 404.html ../../bin/httpd/
	cp index.html ../../bin/httpd/
	cp -R css ../../bin/httpd/
	cp -R js ../../bin/httpd/
	cp -R img ../../bin/httpd/

.c.o:
	$(CC) $(CFLAGS) $< -c -o $@ $(INC)

clean:
	rm -f *.o
	rm -f $(TARGETS)
