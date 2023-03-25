CC=gcc
CPP=g++
CFLAGS=-O3 -c -Wall -DFASTIMAGE_USE_LIBCURL

all: test

test: test.o fastimage.o
	$(CPP) test.o fastimage.o -lcurl -o test
	
test.o: ../test.c
	$(CC) $(CFLAGS) ../test.c
	
fastimage.o: ../fastimage.c
	$(CC) $(CFLAGS) ../fastimage.c
	
clean:
	rm -f *.o test
