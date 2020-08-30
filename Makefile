CC = gcc
CFLAGS = -Wall -g -I .
OBJS = libctemplate/ctemplate.o server/server.o
LIBS = -lsqlite3


run: *.c *.h $(OBJS) 
	$(CC) $(CFLAGS) -c *.c
	$(CC) $(CFLAGS) -o run *.o $(OBJS) $(LIBS)

libctemplate/ctemplate.o: libctemplate/ctemplate.c libctemplate/ctemplate.h

server/server.o: server/server.c server/server.h

clean:
	rm -f $(OBJS) *.o run
