/************************************************************
tcpechoserver.c

This is a concurrent echo server (it echos all input back to 
the client), using tcp protocols.

Copyright (C) 1995 by Fred Sullivan      All Rights Reserved
************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include "errors.h"

#define PORT 25503
#define QUEUELENGTH 5

void echo(int sockfd) {
  char c;
  int status;

  while (read(sockfd, &c, 1) == 1)
    /* Echo the character. */
    if (write(sockfd, &c, 1) != 1)
      fatalerror("can't write to socket");
}
    
main(int argc, char *argv[]) {
  int sockfd, newsockfd;
  struct sockaddr_in clientaddr, serveraddr;
  int clientaddrlength;
  int pid;

  /* Remember the program name for error messages. */
  programname = argv[0];

  /* Open a TCP socket. */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    fatalerror("can't open socket");

  /* Bind the address to the socket. */
  bzero(&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(PORT);
  if (bind(sockfd, (struct sockaddr *) &serveraddr,
           sizeof(serveraddr)) != 0)
    fatalerror("can't bind to socket");
  if (listen(sockfd, QUEUELENGTH) < 0)
    fatalerror("can't listen");

  while (1) {
    /* Wait for a connection. */
    clientaddrlength = sizeof(clientaddr);
    newsockfd = accept(sockfd, 
                       (struct sockaddr *) &clientaddr,
                       &clientaddrlength);
    if (newsockfd < 0)
      fatalerror("accept failure");

    /* Fork a child to handle the connection. */
    pid = fork();
    if (pid < 0)
      fatalerror("fork error");
    else if (pid == 0) {
      /* I'm the child. */
      close(sockfd);
      echo(newsockfd);
      echo(0);
      printf("Escribio\n");
      exit(EXIT_SUCCESS);
    }
    else
      /* I'm the parent. */
      close(newsockfd);
  }
}

