CC = gcc
CFLAGS = -Wall -g -I .
<<<<<<< HEAD
OBJS = libctemplate/ctemplate.o server/server.o
=======
OBJS = libctemplate/ctemplate.o server/server.o jsmn/jsmn.o
>>>>>>> example-app
LIBS = -lsqlite3


run: *.c *.h $(OBJS) 
	$(CC) $(CFLAGS) -c *.c
	$(CC) $(CFLAGS) -o run *.o $(OBJS) $(LIBS)

libctemplate/ctemplate.o: libctemplate/ctemplate.c libctemplate/ctemplate.h

server/server.o: server/server.c server/server.h
<<<<<<< HEAD
=======

jsmn/jsmn.o: jsmn/jsmn.c jsmn/jsmn.h jsmn/utf8.h
>>>>>>> example-app

clean:
	rm -f $(OBJS) *.o run
