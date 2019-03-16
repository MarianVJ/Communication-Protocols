all: build 

build: server client

server: server.o 
	gcc -g server.o -o server

client: client.o 
	gcc -g client.o -o client

.c.o: 
	gcc -g -c $? 

clean:
	-rm -f *.o server client 
	
clean_logs:
	-rm *.log
