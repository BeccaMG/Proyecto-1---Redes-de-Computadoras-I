CC = gcc
CFLAGS = -g -pthread
#LIBS = -lsocket -lnsl

all: schat cchat

errors.o : errors.c errors.h
	$(CC) $(CFLAGS) -c errors.c
	
schat : schat.c errors.o
	$(CC) $(CFLAGS) -o schat schat.c errors.o $(LIBS)

cchat : cchat.c errors.o
	$(CC) $(CFLAGS) -o cchat cchat.c errors.o $(LIBS)

clean:
	rm -f *.o schat cchat
