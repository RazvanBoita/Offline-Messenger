all:
	gcc server.c -o server -lsqlite3
	gcc client.c -o client
clean:
	rm -f server.c client.c