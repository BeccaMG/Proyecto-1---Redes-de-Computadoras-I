CC = gcc
CFLAGS = -g -pthread
#LIBS = -lsocket -lnsl

all : tcpechoserver tcpechoclient 

errors.o : errors.c errors.h
	$(CC) $(CFLAGS) -c errors.c

tcpechoserver : tcpechoserver.c errors.o
	$(CC) $(CFLAGS) -o tcpechoserver tcpechoserver.c errors.o $(LIBS)

tcpechoclient : tcpechoclient.c errors.o
	$(CC) $(CFLAGS) -o tcpechoclient tcpechoclient.c errors.o $(LIBS)


