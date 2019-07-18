all:
	gcc server.c -o server -lpthread -lsqlite3
	gcc client.c -o client

clean:
	rm server client
