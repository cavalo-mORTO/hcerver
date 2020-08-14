CC = gcc
CFLAGS = -Wall -g -I .

libserver.a: server.o
	ar r libserver.a server.o
	ranlib libserver.a
	rm -f server.o

server.o: server.c server.h

clean:
	rm -f *.o *.a
