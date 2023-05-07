all: poll server

poll: pollExample.c
	gcc -o pollExample pollExample.c

server: pollserver.c
	gcc -o pollserver pollserver.c

clean:
	rm pollExample pollserver
