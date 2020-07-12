CC = gcc
CFLAGS = -I .

run: *.c
	$(CC) $(CFLAGS) -o run *.c -L ./libctemplate/ -lctemplate

clean:
	rm -f *.o *.a vgcore.* run
