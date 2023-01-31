all: server subscriber

subscriber: subscriber.c
	gcc subscriber.c  -Wall -lm -o subscriber
	
server: server.c
	gcc server.c  -Wall -lm -o server

clean:
	rm -rf server subscriber