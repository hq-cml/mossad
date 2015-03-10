CC = gcc
CFLAGS  = -fPIC -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -D_REENTRANT
LIB     = -ldl -lrt -lm -lcurl -rdynamic -lcJSON -L.
INC     = -I../../inc -I./

OO  = trapper.o trap_cron.o trap_fun.o trap_net.o
TARGETS = libtrapper.so

all: $(TARGETS)

$(TARGETS): $(OO)
	$(CC) $(CFLAGS) $(OO) -shared -o $@ $(LIBDIR) $(LIB)

install:
	install $(TARGETS) ../../bin/trapper/
	#cp trapper.conf ../../bin/trapper/

.c.o:
	$(CC) $(CFLAGS) $< -c -o $@ $(INC)

clean:
	rm -f *.o
	rm -f $(TARGETS)
