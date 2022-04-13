CC=cc
CFLAGS=-Wall -Wextra -O3
LFLAGS=-lopus -lasound

all: porsuk

porsuk: porsuk.o voip.o xor-feed.o
	$(CC) $(CFLAGS) porsuk.o voip.o xor-feed.o -o porsuk $(LFLAGS) 

porsuk.o: porsuk.c voip.h
	$(CC) $(CFLAGS) -c porsuk.c

voip.o: voip.c voip.h
	$(CC) $(CFLAGS) -c voip.c

xor-feed.o: xor-feed.c xor-feed.h
	$(CC) $(CFLAGS) -c xor-feed.c

clean:
	rm *.o porsuk