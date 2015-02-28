#DEBUG  = -g -D__CJSON_MAIN__
DEBUG  = -g 
CC     = gcc
CFLAGS = $(DEBUG) -Wall -fPIC
LIB    = -lm 

TARGET = libcJSON.a 
#TARGET = demo 

#OBJECT = demo.o 
OBJECT = cJSON.o  

all: $(TARGET) 

$(TARGET):$(OBJECT)
	ar rcs $@ $<

#$(TARGET):$(OBJECT)
#	$(CC) $(CFLAGS) $(LIB) $(OBJECT) -o $@
	
$(OBJECT): %.o:%.c
	$(CC) $(CFLAGS) $< -c -o $@

.PHONY :clean install
install:
	cp cJSON.h ../../demos/trapper/ 
	install $(TARGET) ../../demos/trapper/

clean:
	rm -f *.o
	rm -f *.a
