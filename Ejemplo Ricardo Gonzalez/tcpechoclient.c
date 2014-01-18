/************************************************************
tcpechoclient.c

This is a client for the tcp echo server.  It sends anything
read from standard input to the server, reads the responses,
and sends them to standard output.

Copyright (C) 1995 by Fred Sullivan      All Rights Reserved
************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include "errors.h"

#define PORT 20406

int sockfd;

void copy() {
  int c;
  char outbuffer;
  char inbuffer;

  while ((c = getchar()) != EOF) {
    /* Write a character to the socket. */
    outbuffer = c;
    if (write(sockfd, &outbuffer, 1) != 1)
      fatalerror("can't write to socket");
   
    
  }
}


void *escuchar_socket() {
   
    int c;
    char outbuffer;
    char inbuffer;
    
    while (1) {
        if (read(sockfd, &inbuffer, 1) != 1)
            fatalerror("can't read from socket");
        putchar(inbuffer);
    }
}

main(int argc, char *argv[]) {
  
    struct sockaddr_in serveraddr;
    char *server;

    /* Remember the program name for error messages. */
    programname = argv[0];
    pthread_t hilo_escucha;

    /* Who's the server? */
    if (argc == 2)
        server = argv[1];
    else
        server = "127.0.0.1";

    /* Get the address of the server. */
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(server);
    serveraddr.sin_port = htons(PORT);

    /* Open a socket. */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        fatalerror("can't open socket");

    /* Connect to the server. */
    if (connect(sockfd, (struct sockaddr *) &serveraddr,
                sizeof(serveraddr)) < 0)
        fatalerror("can't connect to server");

    printf("Bienvenido al chat :). Tu nombre es ___.\n");
    printf("El largo máximo de los mensajes es 500 caracteres.\n");
    
    if (pthread_create(&hilo_escucha, NULL, escuchar_socket, NULL)) {
        fatalerror("No se pudo crear un hilo para manejar al cliente.");
    }
    
    /* Copy input to the server. */
    copy(sockfd);
    pthread_kill(hilo_escucha, NULL);
    close(sockfd);

    exit(EXIT_SUCCESS);
}