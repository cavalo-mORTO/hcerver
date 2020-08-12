CC = gcc
CFLAGS = -Wall -g -I .

run: *.c
	$(CC) $(CFLAGS) -o run *.c -L ./libctemplate/ -lctemplate -lsqlite3

clean:
	rm -f *.o *.a vgcore.* run
