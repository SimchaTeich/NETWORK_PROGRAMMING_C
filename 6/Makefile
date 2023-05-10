all: tcp udp

tcp: serverOverTCP.c clientOverTCP.c
	gcc -o serverOverTCP serverOverTCP.c
	gcc -o clientOverTCP clientOverTCP.c

udp: serverOverUDP.c clientOverUDP.c
	gcc -o serverOverUDP serverOverUDP.c
	gcc -o clientOverUDP clientOverUDP.c

clean:
	rm serverOverTCP clientOverTCP serverOverUDP clientOverUDP
