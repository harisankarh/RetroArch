all:
	gcc udpclient.c -o client1 -g -D'PORTNO=5000'
	gcc udpclient.c -o client2 -g -D'PORTNO=5001'
	gcc udpserver.c -o server -g
	gcc thread.c -o thread -lpthread -g -DDEBUG
clean: 
	rm client server thread