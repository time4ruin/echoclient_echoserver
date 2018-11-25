all : client server

client: client.c
	gcc -o client client.c

server: server.c
	gcc -o server server.c -pthread

clean:
	rm -f client
	rm -f server
	rm -f *.o



