CC = gcc
CFLAGS  = -fPIC -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -D_REENTRANT
LIB     = -ldl -lrt -lm -lcurl -rdynamic -lcJSON -L../../lib/cJSON/
INC     = -I../../inc -I../../lib/cJSON/

# vim /etc/ld.so.conf
# add /usr/local/lib
# ldconfig


## export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/:$PKG_CONFIG_PATH
## REDIS   = $(shell /usr/bin/pkg-config --cflags --libs hiredis)
REDIS   = -I/usr/local/include/hiredis -L/usr/local/lib -lhiredis 

OO  = redis_saver.o
TARGETS = libredis_saver.so

all: $(TARGETS)

$(TARGETS): $(OO)
	$(CC) $(REDIS) $(CFLAGS) $(OO) -shared -o $@ $(LIBDIR) $(LIB) 

install:
	install $(TARGETS) ../../bin/redis_saver/
	cp redis_saver.conf ../../bin/redis_saver/

.c.o:
	$(CC) $(REDIS) $(CFLAGS) $< -c -o $@ $(INC) 

clean:
	rm -f *.o
	rm -f $(TARGETS)
