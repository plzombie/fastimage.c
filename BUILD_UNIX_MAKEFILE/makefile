CC=gcc
CPP=g++
CFLAGS=-O3 -c -Wall -DFASTIMAGE_USE_LIBCURL

all: test

test: test.o fastimage.o
	$(CPP) test.o fastimage.o -lcurl -o test
	
test.o:
	$(CC) $(CFLAGS) ../test.c
	
fastimage.o:
	$(CC) $(CFLAGS) ../fastimage.c
	
clean:
	rm -f *.o test
