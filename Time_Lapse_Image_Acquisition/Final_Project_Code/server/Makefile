# _*_ Makefile _*_

CC=gcc

OBJS = server.o 
#OBJC = client.o

all:server #client

server: $(OBJS)
	$(CC) -o server $(OBJS)

server.o: server.c
	$(CC) -c server.c

#client:$(OBJC)
#	$(CC) -o client $(OBJC)

#client.o:client1.c
#	$(CC) -c client.c

clean:
	rm *.o  server
	rm *.ppm
