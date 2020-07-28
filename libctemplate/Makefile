CC = gcc
CFLAGS = -I .

libctemplate.a: ctemplate.o
	ar r libctemplate.a ctemplate.o
	ranlib libctemplate.a
	rm -f ctemplate.o

ctemplate.o: ctemplate.c ctemplate.h

clean:
	rm -f *.o *.a template
