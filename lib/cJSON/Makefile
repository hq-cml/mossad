DEBUG  = -g
CC     = gcc
CFLAGS = $(DEBUG) -Wall -fPIC

TARGET = libcJSON.a 

OBJECT = cJSON.o 

all: $(TARGET) 

$(TARGET):$(OBJECT)
	ar rcs $@ $<

$(OBJECT): %.o:%.c
	$(CC) $(CFLAGS) $(INC) $< -c -o $@

.PHONY :clean install
install:
	cp cJSON.h ../../demos/trapper/ 
	install $(TARGET) ../../demos/trapper/

clean:
	rm -f *.o
	rm -f *.a
