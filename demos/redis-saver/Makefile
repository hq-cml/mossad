CC = gcc
CFLAGS  = -fPIC -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -D_REENTRANT
LIB     = -ldl -lrt -lm -lcurl -rdynamic -lcJSON -L.
INC     = -I../../inc -I../../lib/cJSON/
MONGO   = $(shell /usr/bin/pkg-config --cflags --libs libmongoc-1.0)

OO  = redis_saver.o
TARGETS = libredis_saver.so

all: $(TARGETS)

$(TARGETS): $(OO)
	$(CC) $(MONGO) $(CFLAGS) $(OO) -shared -o $@ $(LIBDIR) $(LIB) 

install:
	install $(TARGETS) ../../bin/redis_saver/
	cp redis_saver.conf ../../bin/redis_saver/

.c.o:
	$(CC) $(MONGO) $(CFLAGS) $< -c -o $@ $(INC) 

clean:
	rm -f *.o
	rm -f $(TARGETS)
