/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFSIZE 10

#ifndef HOSTNAME
#define HOSTNAME "127.0.0.1"
#endif

#ifndef PORTNO
#define PORTNO 5000
#endif 
/* 
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(0);
}

int main(int argc, char **argv) {
  int sockfd, portno, n;
  int serverlen;
  struct sockaddr_in serveraddr;
  struct hostent *server;
  char *hostname = HOSTNAME;
  char buf[BUFSIZE];
  char c;
  /* check command line arguments */
  portno = PORTNO;

  /* socket: create the socket */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* gethostbyname: get the server's DNS entry */
  server = gethostbyname(hostname);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host as %s\n", hostname);
    exit(0);
  }

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
	(char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portno);

  /* get a message from the user */
  bzero(buf, BUFSIZE);
  while(1)
    {
      printf("Please enter msg: ");
      // c = getchar();
      fgets(buf, BUFSIZE, stdin);
      buf[1]='\0';
      /* send the message to the server */
      serverlen = sizeof(serveraddr);
      n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
      if (n < 0) 
	error("ERROR in sendto");
    }
  return 0;
}
